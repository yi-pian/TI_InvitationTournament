#ifndef SIGNAL_TFT_ST7789_FONT_H
#define SIGNAL_TFT_ST7789_FONT_H

#include <stdbool.h>
#include <stdint.h>

#include "signal_tft_st7789.h"

/* Same four printable-ASCII point-matrix sizes as the ILI9341 module. */
typedef enum {
    TFT_ST7789_FONT_6X12 = 0,
    TFT_ST7789_FONT_8X16,
    TFT_ST7789_FONT_12X24,
    TFT_ST7789_FONT_16X32
} tft_st7789_font_t;

#define TFT_ST7789_GLYPH_16X16_BYTES (32U)

/* The supplied font resource has these two example Chinese glyphs. */
extern const uint8_t TFT_ST7789_GLYPH_CN_DIAN_16X16[
    TFT_ST7789_GLYPH_16X16_BYTES];
extern const uint8_t TFT_ST7789_GLYPH_CN_ZI_16X16[
    TFT_ST7789_GLYPH_16X16_BYTES];

tft_st7789_status_t TFT_ST7789_GetFontMetrics(tft_st7789_font_t font,
    uint8_t *width, uint8_t *height);
tft_st7789_status_t TFT_ST7789_DrawChar(tft_st7789_t *tft,
    int32_t x, int32_t y, char character, tft_st7789_font_t font,
    uint16_t foreground, uint16_t background, bool transparent_background);
tft_st7789_status_t TFT_ST7789_DrawString(tft_st7789_t *tft,
    int32_t x, int32_t y, const char *text, tft_st7789_font_t font,
    uint16_t foreground, uint16_t background, bool transparent_background,
    bool wrap);
tft_st7789_status_t TFT_ST7789_DrawInt32(tft_st7789_t *tft,
    int32_t x, int32_t y, int32_t value, tft_st7789_font_t font,
    uint16_t foreground, uint16_t background, bool transparent_background);
tft_st7789_status_t TFT_ST7789_DrawFloat(tft_st7789_t *tft,
    int32_t x, int32_t y, float value, uint8_t decimal_places,
    tft_st7789_font_t font, uint16_t foreground, uint16_t background,
    bool transparent_background);
signal_module_status_t SignalTFTST7789Font_GetModuleStatus(void);

#endif /* SIGNAL_TFT_ST7789_FONT_H */


