#include "lf_tag.h"


/**
* @brief Judgment field status
 */
bool lf_is_field_exists(void) {
    nrf_drv_lpcomp_enable();
    bsp_delay_us(30);                                   // Display for a period of time and sampling to avoid misjudgment
    nrf_lpcomp_task_trigger(NRF_LPCOMP_TASK_SAMPLE);    //Trigger a sampling
    return nrf_lpcomp_result_get() == 1;                //Determine the sampling results of the LF field status
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

    // TAG id broadcast
    nrfx_timer_config_t timer_cfg = NRFX_TIMER_DEFAULT_CONFIG;
    err_code = nrfx_timer_init(&m_timer_send_id, &timer_cfg, timer_ce_handler);
    APP_ERROR_CHECK(err_code);
    nrfx_timer_extended_compare(&m_timer_send_id, NRF_TIMER_CC_CHANNEL2, nrfx_timer_us_to_ticks(&m_timer_send_id, LF_125KHZ_EM410X_BIT_CLOCK), NRF_TIMER_SHORT_COMPARE2_CLEAR_MASK, true);

    if (lf_is_field_exists() && !m_is_lf_emulating) {
        lpcomp_event_handler(NRF_LPCOMP_EVENT_UP);
    }
}

static void lf_sense_disable(void) {
    nrfx_timer_uninit(&m_timer_send_id);    //counterInitializationTimer
    nrfx_lpcomp_uninit();                   //antiInitializationComparator
    m_is_lf_emulating = false;              //setAsNonSimulatedState
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
