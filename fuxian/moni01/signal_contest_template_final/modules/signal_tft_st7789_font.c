#include "signal_tft_st7789_font.h"

#include <stddef.h>

#define TFT_ASCII_FIRST (UINT8_C(0x20))
#define TFT_ASCII_LAST  (UINT8_C(0x7E))

#include "signal_tft_st7789_font_data.inc"

const uint8_t TFT_ST7789_GLYPH_CN_DIAN_16X16[
    TFT_ST7789_GLYPH_16X16_BYTES] = {
    0x80U, 0x00U, 0x80U, 0x00U, 0x80U, 0x00U, 0xFCU, 0x1FU,
    0x84U, 0x10U, 0x84U, 0x10U, 0x84U, 0x10U, 0xFCU, 0x1FU,
    0x84U, 0x10U, 0x84U, 0x10U, 0x84U, 0x10U, 0xFCU, 0x1FU,
    0x84U, 0x50U, 0x80U, 0x40U, 0x80U, 0x40U, 0x00U, 0x7FU
};

const uint8_t TFT_ST7789_GLYPH_CN_ZI_16X16[
    TFT_ST7789_GLYPH_16X16_BYTES] = {
    0x00U, 0x00U, 0xFEU, 0x1FU, 0x00U, 0x08U, 0x00U, 0x04U,
    0x00U, 0x02U, 0x80U, 0x01U, 0x80U, 0x00U, 0xFFU, 0x7FU,
    0x80U, 0x00U, 0x80U, 0x00U, 0x80U, 0x00U, 0x80U, 0x00U,
    0x80U, 0x00U, 0x80U, 0x00U, 0xA0U, 0x00U, 0x40U, 0x00U
};

typedef struct {
    uint8_t width;
    uint8_t height;
    size_t bytes_per_glyph;
    const uint8_t *glyphs;
} tft_st7789_font_descriptor_t;

static const tft_st7789_font_descriptor_t *TFT_ST7789_GetFont(
    tft_st7789_font_t font)
{
    static const tft_st7789_font_descriptor_t fonts[] = {
        {6U, 12U, 12U, &g_font_ascii_6x12[0][0]},
        {8U, 16U, 16U, &g_font_ascii_8x16[0][0]},
        {12U, 24U, 48U, &g_font_ascii_12x24[0][0]},
        {16U, 32U, 64U, &g_font_ascii_16x32[0][0]}
    };

    if ((uint32_t)font >= (sizeof(fonts) / sizeof(fonts[0]))) return NULL;
    return &fonts[(uint32_t)font];
}

tft_st7789_status_t TFT_ST7789_GetFontMetrics(tft_st7789_font_t font,
    uint8_t *width, uint8_t *height)
{
    const tft_st7789_font_descriptor_t *descriptor = TFT_ST7789_GetFont(font);
    if ((descriptor == NULL) || (width == NULL) || (height == NULL)) {
        return TFT_ST7789_ERROR_ARGUMENT;
    }
    *width = descriptor->width;
    *height = descriptor->height;
    return TFT_ST7789_OK;
}

tft_st7789_status_t TFT_ST7789_DrawChar(tft_st7789_t *tft,
    int32_t x, int32_t y, char character, tft_st7789_font_t font,
    uint16_t foreground, uint16_t background, bool transparent_background)
{
    const tft_st7789_font_descriptor_t *descriptor = TFT_ST7789_GetFont(font);
    uint8_t code = (uint8_t)character;
    size_t glyph_index;

    if (descriptor == NULL) return TFT_ST7789_ERROR_ARGUMENT;
    if ((code < TFT_ASCII_FIRST) || (code > TFT_ASCII_LAST)) code = (uint8_t)'?';
    glyph_index = (size_t)(code - TFT_ASCII_FIRST);
    return TFT_ST7789_DrawMonoBitmap(tft, x, y, descriptor->width,
        descriptor->height, descriptor->glyphs +
        glyph_index * descriptor->bytes_per_glyph, descriptor->bytes_per_glyph,
        foreground, background, transparent_background);
}

tft_st7789_status_t TFT_ST7789_DrawString(tft_st7789_t *tft,
    int32_t x, int32_t y, const char *text, tft_st7789_font_t font,
    uint16_t foreground, uint16_t background, bool transparent_background,
    bool wrap)
{
    const tft_st7789_font_descriptor_t *descriptor = TFT_ST7789_GetFont(font);
    int32_t cursor_x = x;
    int32_t cursor_y = y;

    if ((tft == NULL) || (text == NULL) || (descriptor == NULL)) {
        return TFT_ST7789_ERROR_ARGUMENT;
    }
    while (*text != '\0') {
        tft_st7789_status_t status;
        if (*text == '\r') {
            ++text;
            continue;
        }
        if (*text == '\n') {
            cursor_x = x;
            cursor_y += descriptor->height;
            ++text;
            continue;
        }
        if (wrap && ((cursor_x + descriptor->width) >
            (int32_t)TFT_ST7789_GetWidth(tft)) && (cursor_x != x)) {
            cursor_x = x;
            cursor_y += descriptor->height;
        }
        status = TFT_ST7789_DrawChar(tft, cursor_x, cursor_y, *text, font,
            foreground, background, transparent_background);
        if (status != TFT_ST7789_OK) return status;
        cursor_x += descriptor->width;
        ++text;
    }
    return TFT_ST7789_OK;
}

static size_t TFT_ST7789_AppendUint32(char *buffer, size_t position,
    uint32_t value, uint8_t minimum_digits)
{
    char reversed[10];
    size_t count = 0U;
    size_t index;
    do {
        reversed[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while ((value != 0U) || (count < minimum_digits));
    for (index = 0U; index < count; ++index) {
        buffer[position + index] = reversed[count - index - 1U];
    }
    return position + count;
}

tft_st7789_status_t TFT_ST7789_DrawInt32(tft_st7789_t *tft,
    int32_t x, int32_t y, int32_t value, tft_st7789_font_t font,
    uint16_t foreground, uint16_t background, bool transparent_background)
{
    char text[12];
    size_t position = 0U;
    uint32_t magnitude;
    if (value < 0) {
        text[position++] = '-';
        magnitude = (uint32_t)(-(value + 1)) + 1U;
    } else {
        magnitude = (uint32_t)value;
    }
    position = TFT_ST7789_AppendUint32(text, position, magnitude, 1U);
    text[position] = '\0';
    return TFT_ST7789_DrawString(tft, x, y, text, font, foreground,
        background, transparent_background, false);
}

tft_st7789_status_t TFT_ST7789_DrawFloat(tft_st7789_t *tft,
    int32_t x, int32_t y, float value, uint8_t decimal_places,
    tft_st7789_font_t font, uint16_t foreground, uint16_t background,
    bool transparent_background)
{
    char text[32];
    size_t position = 0U;
    float magnitude;
    uint32_t whole;
    uint32_t fractional;
    uint32_t scale = 1U;
    uint8_t index;

    if (decimal_places > 6U) return TFT_ST7789_ERROR_ARGUMENT;
    if (value != value) return TFT_ST7789_DrawString(tft, x, y, "nan", font,
        foreground, background, transparent_background, false);
    if (value < 0.0f) {
        text[position++] = '-';
        magnitude = -value;
    } else {
        magnitude = value;
    }
    if (magnitude > 4294967040.0f) return TFT_ST7789_ERROR_ARGUMENT;
    for (index = 0U; index < decimal_places; ++index) scale *= 10U;
    whole = (uint32_t)magnitude;
    fractional = (uint32_t)(((magnitude - (float)whole) * (float)scale) + 0.5f);
    if (fractional >= scale) {
        if (whole == UINT32_MAX) return TFT_ST7789_ERROR_ARGUMENT;
        ++whole;
        fractional = 0U;
    }
    position = TFT_ST7789_AppendUint32(text, position, whole, 1U);
    if (decimal_places != 0U) {
        text[position++] = '.';
        position = TFT_ST7789_AppendUint32(text, position, fractional,
            decimal_places);
    }
    text[position] = '\0';
    return TFT_ST7789_DrawString(tft, x, y, text, font, foreground,
        background, transparent_background, false);
}

signal_module_status_t SignalTFTST7789Font_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}


