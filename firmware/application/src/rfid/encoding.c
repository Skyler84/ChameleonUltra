#include "encoding.h"

void manchester_encoder_init(manchester_encoder_t *enc, const uint8_t *data, uint8_t num_bits, bool inverted) {
  enc->data_bits = data;
  enc->num_bits = num_bits;
  enc->clk_phase = false;
  enc->bitpos = 0;
  enc->inverted = inverted;
}

static uint8_t manchester_biphase_decode_helper(RAWBUF_TYPE_S *Pdata, const uint8_t *lut_vars, uint8_t state) {
  const uint8_t lut_state[] = {0, 4, 0, -1, 4, 0, -1, -1};
  for (int i = Pdata->startbit; i < RAW_BUF_SIZE * 8; i++) {
    uint8_t thisbit = readbit(Pdata->rawa, Pdata->rawb, i);
    uint8_t value = lut_vars[state+thisbit];
    if (value == 0) return 0;
    for(; value > 1; value >>=1) {
      writebit(Pdata->hexbuf, Pdata->hexbuf, i, value & 1);
    }
    state = lut_state[state+thisbit];
  }
  return 1;
}

uint8_t manchester_decode(RAWBUF_TYPE_S *Pdata) {
    const uint8_t lut_vars_pos[] = {3, 2, 6, 0, 2, 6, 0, 0};
    // const uint8_t lut_vars_neg[] = {2, 3, 5, 0, 3, 5, 0, 0};
    /**
     *  0_(10)_10    or 01_(01)_01
     *  0_(10_01)_0x or 01_(10)_10_xx
     *                  01_(10_01)_xx
     * 
     */
    return manchester_biphase_decode_helper(Pdata, lut_vars_pos, 0);
}

uint8_t biphase_decode(RAWBUF_TYPE_S *Pdata) {
    const uint8_t lut_vars_pos[] = {2, 6, 7, 0, 2, 3, 0, 0};
    // const uint8_t lut_vars_neg[] = {3, 5, 4, 0, 3, 2, 0, 0};
    /**
     *  01_(01)_0    or 0_(10)_1x
     *  01_(00)_10   or 0_(11_01)_0x
     *                  0_(11_00)_1x
     * 
     */
    return manchester_biphase_decode_helper(Pdata, lut_vars_pos, 0);
}