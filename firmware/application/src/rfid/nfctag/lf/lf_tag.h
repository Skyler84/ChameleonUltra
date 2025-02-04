#ifndef __LF_TAG_H
#define __LF_TAG_H

#include <stdbool.h>
#include "rfid_main.h"
#include "tag_emulation.h"

#include "nrf_drv_timer.h"

extern volatile bool m_is_lf_emulating;
extern const nrfx_timer_t m_lf_tag_timer
extern nrfx_timer_config_t m_lf_tag_timer_cfg;

/**
 * Low -frequency analog card adjustment Manchester signal
 * The definition of the packaging tool macro only needs to be modulated 0 and 1
 */
#define LF_125KHZ_BROADCAST_MAX     10      // 32.768ms once, about 31 times in one second


void lf_tag_125khz_sense_switch(bool enable);
bool lf_is_field_exists(void);

#endif
