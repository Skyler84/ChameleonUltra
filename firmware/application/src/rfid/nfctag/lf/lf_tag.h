#ifndef __LF_TAG_H
#define __LF_TAG_H

#include <stdbool.h>
#include "rfid_main.h"
#include "tag_emulation.h"

#include "nrf_drv_timer.h"
#include "nrf_drv_gpiote.h"

extern volatile bool m_is_lf_emulating;
extern const nrfx_timer_t m_lf_tag_timer;
extern nrfx_timer_config_t m_lf_tag_timer_cfg;

// Get the specified position bit
#define GETBIT(v, bit) ((v >> bit) & 0x01)
// Antenna control
#define ANT_TO_MOD()   nrf_gpio_pin_set(LF_MOD)
#define ANT_NO_MOD()  nrf_gpio_pin_clear(LF_MOD)

/**
 * Low -frequency analog card adjustment Manchester signal
 * The definition of the packaging tool macro only needs to be modulated 0 and 1
 */
#define LF_125KHZ_BROADCAST_MAX     10      // 32.768ms once, about 31 times in one second

struct lf_tag_decode_config
{
    void (*gpioe_handler)(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
};

typedef struct 
{
    bool reader_talks_first;
} tag_lf_config_t;
typedef struct{
    void (*field_up)(void);
    void (*sense_enabled)(bool);
} tag_lf_handler_t;

// extern tag_lf_handler_t m_tag_lf_handler;

void lf_tag_config_clear();
void lf_tag_set_handler(tag_lf_handler_t *handler);

void lf_tag_125khz_sense_switch(bool enable);
bool lf_is_field_exists(void);

#endif
