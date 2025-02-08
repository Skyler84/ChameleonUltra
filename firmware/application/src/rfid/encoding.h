#ifndef _ENCODING_H
#define _ENCODING_H

#include <stdint.h>
#include <stdbool.h>

#include "lf_em410x_data.h"

typedef struct {
  const uint8_t *data_bits;
  uint8_t num_bits;
  bool clk_phase;
  uint8_t bitpos;
  bool inverted;
} manchester_encoder_t;

void manchester_encoder_init(manchester_encoder_t *enc, const uint8_t *data, uint8_t num_bits, bool inverted);

uint8_t manchester_decode(RAWBUF_TYPE_S *Pdata);

#endif