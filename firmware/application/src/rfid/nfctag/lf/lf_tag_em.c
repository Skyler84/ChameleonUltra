#include <stdint.h>

#include "lf_tag_em.h"
#include "syssleep.h"
#include "tag_emulation.h"
#include "fds_util.h"
#include "tag_persistence.h"
#include "bsp_delay.h"
#include "data_utils.h"
#include "encoding.h"

#include "nrf_gpio.h"
#include "nrf_drv_lpcomp.h"

#define NRF_LOG_MODULE_NAME tag_em410x
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();


// Bit data carrying 64 -bit ID number
static uint64_t m_id_bit_data = 0;
// The current broadcast ID number is 33ms every few times, and can be broadcast about 30 times a second
static uint8_t m_send_id_count;
// Cache label type
static tag_specific_type_t m_tag_type = TAG_TYPE_UNDEFINED;
// Manchester encoder state
static manchester_encoder_t encoder;

    
/**
 * @brief Convert the card number of EM410X to the memory layout of U64 and calculate the puppet school inspection
 *  According to the instructions of the manual, EM4100 is sufficient to accommodate U64
 */
uint64_t em410x_id_to_memory64(uint8_t id[5]) {
    //Union, what you see is obtained
    union {
        uint64_t u64;
        struct {
            // 9 header bits
            uint8_t h00: 1;
            uint8_t h01: 1;
            uint8_t h02: 1;
            uint8_t h03: 1;
            uint8_t h04: 1;
            uint8_t h05: 1;
            uint8_t h06: 1;
            uint8_t h07: 1;
            uint8_t h08: 1;
            // 8 version bits and 2 bit parity
            uint8_t d00: 1;
            uint8_t d01: 1;
            uint8_t d02: 1;
            uint8_t d03: 1;
            uint8_t p0: 1;
            uint8_t d10: 1;
            uint8_t d11: 1;
            uint8_t d12: 1;
            uint8_t d13: 1;
            uint8_t p1: 1;
            // 32 data bits and 8 bit parity
            uint8_t d20: 1;
            uint8_t d21: 1;
            uint8_t d22: 1;
            uint8_t d23: 1;
            uint8_t p2: 1;
            uint8_t d30: 1;
            uint8_t d31: 1;
            uint8_t d32: 1;
            uint8_t d33: 1;
            uint8_t p3: 1;
            uint8_t d40: 1;
            uint8_t d41: 1;
            uint8_t d42: 1;
            uint8_t d43: 1;
            uint8_t p4: 1;
            uint8_t d50: 1;
            uint8_t d51: 1;
            uint8_t d52: 1;
            uint8_t d53: 1;
            uint8_t p5: 1;
            uint8_t d60: 1;
            uint8_t d61: 1;
            uint8_t d62: 1;
            uint8_t d63: 1;
            uint8_t p6: 1;
            uint8_t d70: 1;
            uint8_t d71: 1;
            uint8_t d72: 1;
            uint8_t d73: 1;
            uint8_t p7: 1;
            uint8_t d80: 1;
            uint8_t d81: 1;
            uint8_t d82: 1;
            uint8_t d83: 1;
            uint8_t p8: 1;
            uint8_t d90: 1;
            uint8_t d91: 1;
            uint8_t d92: 1;
            uint8_t d93: 1;
            uint8_t p9: 1;
            // 5 bit end.
            uint8_t pc0: 1;
            uint8_t pc1: 1;
            uint8_t pc2: 1;
            uint8_t pc3: 1;
            uint8_t s0: 1;
        } bit;
    } memory;

    // Okay, it's the most critical time at present, and now you need to assign and calculate the Qiqi school inspection
    // 1. First assign the front guide code
    memory.bit.h00 = memory.bit.h01 = memory.bit.h02 =
                                          memory.bit.h03 = memory.bit.h04 = memory.bit.h05 =
                                                               memory.bit.h06 = memory.bit.h07 = memory.bit.h08 = 1;
    //2. Assign the 8bit version or custom ID
    memory.bit.d00 = GETBIT(id[0], 7);
    memory.bit.d01 = GETBIT(id[0], 6);
    memory.bit.d02 = GETBIT(id[0], 5);
    memory.bit.d03 = GETBIT(id[0], 4);
    memory.bit.p0 = memory.bit.d00 ^ memory.bit.d01 ^ memory.bit.d02 ^ memory.bit.d03;
    memory.bit.d10 = GETBIT(id[0], 3);
    memory.bit.d11 = GETBIT(id[0], 2);
    memory.bit.d12 = GETBIT(id[0], 1);
    memory.bit.d13 = GETBIT(id[0], 0);
    memory.bit.p1 = memory.bit.d10 ^ memory.bit.d11 ^ memory.bit.d12 ^ memory.bit.d13;
    // 3. Assign the data of 32Bit
    // -byte1
    memory.bit.d20 = GETBIT(id[1], 7);
    memory.bit.d21 = GETBIT(id[1], 6);
    memory.bit.d22 = GETBIT(id[1], 5);
    memory.bit.d23 = GETBIT(id[1], 4);
    memory.bit.p2 = memory.bit.d20 ^ memory.bit.d21 ^ memory.bit.d22 ^ memory.bit.d23;
    memory.bit.d30 = GETBIT(id[1], 3);
    memory.bit.d31 = GETBIT(id[1], 2);
    memory.bit.d32 = GETBIT(id[1], 1);
    memory.bit.d33 = GETBIT(id[1], 0);
    memory.bit.p3 = memory.bit.d30 ^ memory.bit.d31 ^ memory.bit.d32 ^ memory.bit.d33;
    // - byte2
    memory.bit.d40 = GETBIT(id[2], 7);
    memory.bit.d41 = GETBIT(id[2], 6);
    memory.bit.d42 = GETBIT(id[2], 5);
    memory.bit.d43 = GETBIT(id[2], 4);
    memory.bit.p4 = memory.bit.d40 ^ memory.bit.d41 ^ memory.bit.d42 ^ memory.bit.d43;
    memory.bit.d50 = GETBIT(id[2], 3);
    memory.bit.d51 = GETBIT(id[2], 2);
    memory.bit.d52 = GETBIT(id[2], 1);
    memory.bit.d53 = GETBIT(id[2], 0);
    memory.bit.p5 = memory.bit.d50 ^ memory.bit.d51 ^ memory.bit.d52 ^ memory.bit.d53;
    // - byte3
    memory.bit.d60 = GETBIT(id[3], 7);
    memory.bit.d61 = GETBIT(id[3], 6);
    memory.bit.d62 = GETBIT(id[3], 5);
    memory.bit.d63 = GETBIT(id[3], 4);
    memory.bit.p6 = memory.bit.d60 ^ memory.bit.d61 ^ memory.bit.d62 ^ memory.bit.d63;
    memory.bit.d70 = GETBIT(id[3], 3);
    memory.bit.d71 = GETBIT(id[3], 2);
    memory.bit.d72 = GETBIT(id[3], 1);
    memory.bit.d73 = GETBIT(id[3], 0);
    memory.bit.p7 = memory.bit.d70 ^ memory.bit.d71 ^ memory.bit.d72 ^ memory.bit.d73;
    // - byte4
    memory.bit.d80 = GETBIT(id[4], 7);
    memory.bit.d81 = GETBIT(id[4], 6);
    memory.bit.d82 = GETBIT(id[4], 5);
    memory.bit.d83 = GETBIT(id[4], 4);
    memory.bit.p8 = memory.bit.d80 ^ memory.bit.d81 ^ memory.bit.d82 ^ memory.bit.d83;
    memory.bit.d90 = GETBIT(id[4], 3);
    memory.bit.d91 = GETBIT(id[4], 2);
    memory.bit.d92 = GETBIT(id[4], 1);
    memory.bit.d93 = GETBIT(id[4], 0);
    memory.bit.p9 = memory.bit.d90 ^ memory.bit.d91 ^ memory.bit.d92 ^ memory.bit.d93;
    // 4. Calculate the vertical puppet verification
    memory.bit.pc0 = memory.bit.d00 ^ memory.bit.d10 ^ memory.bit.d20 ^ memory.bit.d30 ^ memory.bit.d40 ^ memory.bit.d50 ^ memory.bit.d60 ^ memory.bit.d70 ^ memory.bit.d80 ^ memory.bit.d90;
    memory.bit.pc1 = memory.bit.d01 ^ memory.bit.d11 ^ memory.bit.d21 ^ memory.bit.d31 ^ memory.bit.d41 ^ memory.bit.d51 ^ memory.bit.d61 ^ memory.bit.d71 ^ memory.bit.d81 ^ memory.bit.d91;
    memory.bit.pc2 = memory.bit.d02 ^ memory.bit.d12 ^ memory.bit.d22 ^ memory.bit.d32 ^ memory.bit.d42 ^ memory.bit.d52 ^ memory.bit.d62 ^ memory.bit.d72 ^ memory.bit.d82 ^ memory.bit.d92;
    memory.bit.pc3 = memory.bit.d03 ^ memory.bit.d13 ^ memory.bit.d23 ^ memory.bit.d33 ^ memory.bit.d43 ^ memory.bit.d53 ^ memory.bit.d63 ^ memory.bit.d73 ^ memory.bit.d83 ^ memory.bit.d93;
    //5. Set the position of the last EOF, this wave of conversion is over
    memory.bit.s0 = 0;
    //Return to the U64 data in the combination, this is the data we finally need,
    // In the later stage analog card, just take out each bit to send it
    return memory.u64;
}

/**
 * @brief Modulate single bit of data, manchester encoded.
 * @return 0: data remaining
 *        1: data finished
 */
uint8_t manchester_encoder_txbit(manchester_encoder_t *enc) {
    bool bit = bitplane_readbits(enc->bitpos, 1, enc->data_bits);
    bool mod = bit ^ enc->clk_phase ^ enc->inverted;
    if (mod) {
        ANT_TO_MOD();
    } else {
        ANT_NO_MOD();
    }
    enc->clk_phase = !enc->clk_phase;
    if (enc->clk_phase == false) {
        enc->bitpos++;
    } else if (enc->bitpos == enc->num_bits) {
        return 1;
    }
    return 0;
}

void timer_ce_handler(nrf_timer_event_t event_type, void *p_context) {
    if (event_type != NRF_TIMER_EVENT_COMPARE2) {
        return;
    }

    if (manchester_encoder_txbit(&encoder) == 0) {
        return;
    }
    encoder.bitpos = 0;
    m_send_id_count++;
    if (m_send_id_count < LF_125KHZ_BROADCAST_MAX) {
        return;
    }        
    nrfx_timer_disable(&m_lf_tag_timer);                       // Close the timer of the broadcast venue
    // We don't need any events, but only need to detect the state of the field
    NRF_LPCOMP->INTENCLR = LPCOMP_INTENCLR_CROSS_Msk | LPCOMP_INTENCLR_UP_Msk | LPCOMP_INTENCLR_DOWN_Msk | LPCOMP_INTENCLR_READY_Msk;
    if (lf_is_field_exists()) {
        nrf_drv_lpcomp_disable();
        nrfx_timer_enable(&m_lf_tag_timer);                    // Open the timer of the broadcaster and continue to simulate
    } else {
        // Open the incident interruption, so that the next event can be in and out normally
        g_is_tag_emulating = false;                             // Reset the flag in the simulation
        m_is_lf_emulating = false;
        TAG_FIELD_LED_OFF()                                     // Make sure the indicator light of the LF field status
        NRF_LPCOMP->INTENSET = LPCOMP_INTENCLR_CROSS_Msk | LPCOMP_INTENCLR_UP_Msk | LPCOMP_INTENCLR_DOWN_Msk | LPCOMP_INTENCLR_READY_Msk;
        // call sleep_timer_start *after* unsetting g_is_tag_emulating
        sleep_timer_start(SLEEP_DELAY_MS_FIELD_125KHZ_LOST);    // Start the timer to enter the sleep
        NRF_LOG_INFO("LF FIELD LOST");
    }
}

static void em41_field_up_handler(void) {
    // Open the timer of the broadcast card number
    nrfx_timer_enable(&m_lf_tag_timer);
}

static void em41_sense_enabled_handler(bool enabled) {
    if (!enabled) return;
    ret_code_t err_code;
    // TAG id broadcast
    nrfx_timer_config_t timer_cfg = NRFX_TIMER_DEFAULT_CONFIG;
    err_code = nrfx_timer_init(&m_lf_tag_timer, &timer_cfg, timer_ce_handler);
    APP_ERROR_CHECK(err_code);
    nrfx_timer_extended_compare(&m_lf_tag_timer, NRF_TIMER_CC_CHANNEL2, nrfx_timer_us_to_ticks(&m_lf_tag_timer, LF_125KHZ_EM410X_BIT_CLOCK), NRF_TIMER_SHORT_COMPARE2_CLEAR_MASK, true);
}


/** @brief EM410X load data
 * @param type     Refined label type
 * @param buffer   Data buffer
 */
int lf_tag_em410x_data_loadcb(tag_specific_type_t type, tag_data_buffer_t *buffer) {
    //Make sure that external capacity is enough to convert to an information structure
    if (buffer->length >= LF_EM410X_TAG_ID_SIZE) {
        // The ID card number is directly converted here as the corresponding BIT data stream
        m_tag_type = type;
        m_id_bit_data = em410x_id_to_memory64(buffer->buffer);
        manchester_encoder_init(&encoder, (uint8_t *)&m_id_bit_data, 64, false);
        tag_lf_handler_t handler = {
            .sense_enabled = em41_sense_enabled_handler,
            .field_up = em41_field_up_handler,
        };
        lf_tag_set_handler(&handler);
        NRF_LOG_INFO("LF Em410x data load finish.");
    } else {
        NRF_LOG_ERROR("LF_EM410X_TAG_ID_SIZE too big.");
    }
    return LF_EM410X_TAG_ID_SIZE;
}

/** @brief Id card deposit card number before callback
 * @param type      Refined label type
 * @param buffer    Data buffer
 * @return The length of the data that needs to be saved is that it does not save when 0
 */
int lf_tag_em410x_data_savecb(tag_specific_type_t type, tag_data_buffer_t *buffer) {
    // Make sure to load this label before allowing saving
    if (m_tag_type != TAG_TYPE_UNDEFINED) {
        // Just save the original card package directly
        return LF_EM410X_TAG_ID_SIZE;
    } else {
        return 0;
    }
}

/** @brief Id card deposit card number before callback
 * @param slot     Card slot number
 * @param tag_type  Refined label type
 * @return Whether the format is successful, if the formatting is successful, it will return to True, otherwise False will be returned
 */
bool lf_tag_em410x_data_factory(uint8_t slot, tag_specific_type_t tag_type) {
    // default id, must to align(4), more word...
    uint8_t tag_id[5] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x88 };
    // Write the data in Flash
    tag_sense_type_t sense_type = get_sense_type_from_tag_type(tag_type);
    fds_slot_record_map_t map_info; // Get the special card slot FDS record information
    get_fds_map_by_slot_sense_type_for_dump(slot, sense_type, &map_info);
    //Call the blocked FDS to write the function, and write the data of the specified field type of the card slot into the Flash
    bool ret = fds_write_sync(map_info.id, map_info.key, sizeof(tag_id), (uint8_t *)tag_id);
    if (ret) {
        NRF_LOG_INFO("Factory slot data success.");
    } else {
        NRF_LOG_ERROR("Factory slot data error.");
    }
    return ret;
}
