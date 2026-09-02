#ifndef SIGNAL_TFT_ST7789_TEXT_H
#define SIGNAL_TFT_ST7789_TEXT_H

#include <stdint.h>

#include "signal_tft_st7789.h"

signal_result_t SignalTFTST7789Text_DrawChar(tft_st7789_t *tft,
    int32_t x, int32_t y, char symbol, uint8_t scale,
    uint16_t foreground, uint16_t background);
signal_result_t SignalTFTST7789Text_DrawString(tft_st7789_t *tft,
    int32_t x, int32_t y, const char *text, uint8_t scale,
    uint16_t foreground, uint16_t background);
signal_result_t SignalTFTST7789Text_DrawUint(tft_st7789_t *tft,
    int32_t x, int32_t y, uint32_t value, uint8_t digits, uint8_t scale,
    uint16_t foreground, uint16_t background);
signal_module_status_t SignalTFTST7789Text_GetModuleStatus(void);

#endif
