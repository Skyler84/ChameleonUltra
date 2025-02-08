#include <stdint.h>

#include "lf_tag.h"
#include "syssleep.h"
#include "tag_emulation.h"
#include "fds_util.h"
#include "tag_persistence.h"
#include "bsp_delay.h"

#include "nrf_gpio.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_lpcomp.h"

#define NRF_LOG_MODULE_NAME tag
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();


// Whether the USB light effect is allowed to enable
extern bool g_usb_led_marquee_enable;

// Whether it is currently in the low -frequency card number of broadcasting
volatile bool m_is_lf_emulating = false;
// The timer of the delivery card number, we use the timer 3
const nrfx_timer_t m_lf_tag_timer = NRFX_TIMER_INSTANCE(3);
// Config for the emulation timer
nrfx_timer_config_t m_lf_tag_timer_cfg = NRFX_TIMER_DEFAULT_CONFIG;
// Config for tag handlers
tag_lf_handler_t m_tag_lf_handler;

/**
* @brief Judgment field status
 */
bool lf_is_field_exists(void) {
    nrf_drv_lpcomp_enable();
    bsp_delay_us(30);                                   // Display for a period of time and sampling to avoid misjudgment
    nrf_lpcomp_task_trigger(NRF_LPCOMP_TASK_SAMPLE);    //Trigger a sampling
    return nrf_lpcomp_result_get() == 1;                //Determine the sampling results of the LF field status
}


/**
 * @brief LPCOMP event handler is called when LPCOMP detects voltage drop.
 *
 * This function is called from interrupt context so it is very important
 * to return quickly. Don't put busy loops or any other CPU intensive actions here.
 * It is also not allowed to call soft device functions from it (if LPCOMP IRQ
 * priority is set to APP_IRQ_PRIORITY_HIGH).
 */
static void lpcomp_event_handler(nrf_lpcomp_event_t event) {
    // Only when the low -frequency simulation is not launched, and the analog card is started
    if (!m_is_lf_emulating && event == NRF_LPCOMP_EVENT_UP) {
        // Turn off dormant delay
        sleep_timer_stop();
        // Close the comparator
        nrf_drv_lpcomp_disable();

        // Set the simulation status logo bit
        m_is_lf_emulating = true;
        g_is_tag_emulating = true;

        // Simulation card status should be turned off the USB light effect
        g_usb_led_marquee_enable = false;

        // LED status update
        set_slot_light_color(RGB_BLUE);
        TAG_FIELD_LED_ON()

        if (m_tag_lf_handler.field_up) {
            m_tag_lf_handler.field_up();
        }

        //In any case, every time the state finds changes, you need to reset the BIT location of the sending
        // m_send_id_count = 0;
        // m_bit_send_position = 0;
        // m_is_send_first_edge = true;

        // openThePreciseHardwareTimerToTheBroadcastCardNumber
        // nrfx_timer_enable(&m_lf_tag_timer);

        NRF_LOG_INFO("LF FIELD DETECTED");
    }
}

static void lf_sense_enable(void) {
    ret_code_t err_code;

    nrf_drv_lpcomp_config_t config = NRF_DRV_LPCOMP_DEFAULT_CONFIG;
    config.hal.reference = NRF_LPCOMP_REF_SUPPLY_1_16;
    config.input = LF_RSSI;
    config.hal.detection = NRF_LPCOMP_DETECT_UP;
    config.hal.hyst = NRF_LPCOMP_HYST_50mV;

    err_code = nrf_drv_lpcomp_init(&config, lpcomp_event_handler);
    APP_ERROR_CHECK(err_code);

    if (m_tag_lf_handler.sense_enabled) {
        m_tag_lf_handler.sense_enabled(true);
    }

    if (lf_is_field_exists() && !m_is_lf_emulating) {
        lpcomp_event_handler(NRF_LPCOMP_EVENT_UP);
    }
}

static void lf_sense_disable(void) {
    nrfx_timer_uninit(&m_lf_tag_timer);    //counterInitializationTimer
    nrfx_lpcomp_uninit();                   //antiInitializationComparator
    m_is_lf_emulating = false;              //setAsNonSimulatedState
}

void lf_tag_config_clear() {
    // m_tag_lf_config.reader_talks_first = false;
    // m_tag_lf_config.field_up_handler = NULL;
}

void lf_tag_set_handler(tag_lf_handler_t *handler) {
    memcpy(&m_tag_lf_handler, handler, sizeof(tag_lf_handler_t));
}

static enum  {
    LF_SENSE_STATE_NONE,
    LF_SENSE_STATE_DISABLE,
    LF_SENSE_STATE_ENABLE,
} m_lf_sense_state = LF_SENSE_STATE_NONE;

/**
 * @brief switchLfFieldInductionToEnableTheState
 */
void lf_tag_125khz_sense_switch(bool enable) {
    // initializationModulationFootIsOutput
    nrf_gpio_cfg_output(LF_MOD);
    //theDefaultIsNotShortCircuitAntenna (shortCircuitWillCauseRssiToBeUnableToJudge)
    ANT_NO_MOD();

    //forTheFirstTimeOrDisabled,OnlyInitializationIsAllowed
    if (m_lf_sense_state == LF_SENSE_STATE_NONE || m_lf_sense_state == LF_SENSE_STATE_DISABLE) {
        if (enable) {
            m_lf_sense_state = LF_SENSE_STATE_ENABLE;
            lf_sense_enable();
        }
    } else {    // inOtherCases,OnlyAntiInitializationIsAllowed
        if (!enable) {
            m_lf_sense_state = LF_SENSE_STATE_DISABLE;
            lf_sense_disable();
        }
    }
}
