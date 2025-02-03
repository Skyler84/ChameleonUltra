#ifndef __LF_TAG_HITAG_H
#define __LF_TAG_HITAG_H

#include <stdbool.h>
#include "rfid_main.h"
#include "tag_emulation.h"


/**
 * Low -frequency analog card adjustment Manchester signal
 * The definition of the packaging tool macro only needs to be modulated 0 and 1
 */



int lf_tag_hitag_data_loadcb(tag_specific_type_t type, tag_data_buffer_t *buffer);
int lf_tag_hitag_data_savecb(tag_specific_type_t type, tag_data_buffer_t *buffer);
bool lf_tag_hitag_data_factory(uint8_t slot, tag_specific_type_t tag_type);

#endif
