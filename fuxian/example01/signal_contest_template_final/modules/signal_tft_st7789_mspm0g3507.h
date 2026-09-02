#ifndef SIGNAL_TFT_ST7789_MSPM0G3507_H
#define SIGNAL_TFT_ST7789_MSPM0G3507_H

#include "signal_tft_st7789.h"

tft_st7789_status_t SignalTFTST7789_MSPM0_Init(
    tft_st7789_t *tft, tft_st7789_rotation_t rotation,
    uint16_t x_offset, uint16_t y_offset);

#endif /* SIGNAL_TFT_ST7789_MSPM0G3507_H */
