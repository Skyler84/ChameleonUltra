#ifndef __LF_TAG_EM_H
#define __LF_TAG_EM_H

#include "lf_tag.h"


/**
 * Low -frequency analog card adjustment Manchester signal
 * The definition of the packaging tool macro only needs to be modulated 0 and 1
 */
#define LF_125KHZ_EM410X_BIT_SIZE   64
#define LF_125KHZ_EM410X_BIT_CLOCK  256 /*uS (32/125kHz) or is it 16us?*/
#define LF_EM410X_TAG_ID_SIZE       5


int lf_tag_em410x_data_loadcb(tag_specific_type_t type, tag_data_buffer_t *buffer);
int lf_tag_em410x_data_savecb(tag_specific_type_t type, tag_data_buffer_t *buffer);
bool lf_tag_em410x_data_factory(uint8_t slot, tag_specific_type_t tag_type);

#endif
