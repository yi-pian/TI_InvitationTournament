#ifndef SIGNAL_TFT_ILI9341_H
#define SIGNAL_TFT_ILI9341_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "signal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TFT_ILI9341_NATIVE_WIDTH   240U
#define TFT_ILI9341_NATIVE_HEIGHT  320U

/* RGB565 colors. TFT_ILI9341_RGB565() can be used for custom colors. */
#define TFT_ILI9341_BLACK    UINT16_C(0x0000)
#define TFT_ILI9341_WHITE    UINT16_C(0xFFFF)
#define TFT_ILI9341_RED      UINT16_C(0xF800)
#define TFT_ILI9341_GREEN    UINT16_C(0x07E0)
#define TFT_ILI9341_BLUE     UINT16_C(0x001F)
#define TFT_ILI9341_CYAN     UINT16_C(0x07FF)
#define TFT_ILI9341_MAGENTA  UINT16_C(0xF81F)
#define TFT_ILI9341_YELLOW   UINT16_C(0xFFE0)

#define TFT_ILI9341_RGB565(red, green, blue) \
    ((uint16_t)((((uint16_t)(red) & UINT16_C(0xF8)) << 8U) | \
                (((uint16_t)(green) & UINT16_C(0xFC)) << 3U) | \
                (((uint16_t)(blue)) >> 3U)))

typedef signal_result_t tft_ili9341_status_t;

#define TFT_ILI9341_OK                     SIGNAL_RESULT_OK
#define TFT_ILI9341_ERROR_ARGUMENT         SIGNAL_RESULT_INVALID_ARGUMENT
#define TFT_ILI9341_ERROR_IO               SIGNAL_RESULT_HARDWARE_ERROR
#define TFT_ILI9341_ERROR_NOT_INITIALIZED  SIGNAL_RESULT_NOT_INITIALIZED

typedef enum {
    TFT_ILI9341_ROTATION_0 = 0,
    TFT_ILI9341_ROTATION_180 = 1,
    TFT_ILI9341_ROTATION_270 = 2,
    TFT_ILI9341_ROTATION_90 = 3
} tft_ili9341_rotation_t;

/* Built-in printable ASCII fonts. Width is half of height. */
typedef enum {
    TFT_ILI9341_FONT_6X12 = 0,
    TFT_ILI9341_FONT_8X16 = 1,
    TFT_ILI9341_FONT_12X24 = 2,
    TFT_ILI9341_FONT_16X32 = 3
} tft_ili9341_font_t;

#define TFT_ILI9341_GLYPH_16X16_BYTES  32U

/* The supplied source contains these two sample Chinese glyphs only. */
extern const uint8_t TFT_ILI9341_GLYPH_CN_DIAN_16X16[
    TFT_ILI9341_GLYPH_16X16_BYTES];
extern const uint8_t TFT_ILI9341_GLYPH_CN_ZI_16X16[
    TFT_ILI9341_GLYPH_16X16_BYTES];

/*
 * Platform callbacks. Returning 0 from write means success.
 * GPIO callbacks receive true for a high level and false for a low level.
 * lock/unlock are optional; supply them when the SPI bus is shared by tasks.
 */
typedef int  (*tft_ili9341_write_fn)(void *context,
                                     const uint8_t *data,
                                     size_t length);
typedef void (*tft_ili9341_gpio_fn)(void *context, bool high);
typedef void (*tft_ili9341_delay_fn)(void *context, uint32_t milliseconds);
typedef void (*tft_ili9341_sync_fn)(void *context);

typedef struct {
    void *context;
    tft_ili9341_write_fn write;
    /* Optional when the SPI adapter controls CS in hardware. */
    tft_ili9341_gpio_fn set_cs;
    tft_ili9341_gpio_fn set_dc;
    tft_ili9341_gpio_fn set_reset;       /* Optional if reset is tied high. */
    tft_ili9341_gpio_fn set_backlight;   /* Optional. true means light on. */
    tft_ili9341_delay_fn delay_ms;
    tft_ili9341_sync_fn lock;            /* Optional. */
    tft_ili9341_sync_fn unlock;          /* Optional. */
} tft_ili9341_config_t;

typedef struct {
    tft_ili9341_config_t config;
    uint16_t width;
    uint16_t height;
    tft_ili9341_rotation_t rotation;
    bool initialized;
} tft_ili9341_t;

tft_ili9341_status_t TFT_ILI9341_Init(tft_ili9341_t *tft,
                                      const tft_ili9341_config_t *config,
                                      tft_ili9341_rotation_t rotation);
tft_ili9341_status_t TFT_ILI9341_SetRotation(
    tft_ili9341_t *tft,
    tft_ili9341_rotation_t rotation);
void TFT_ILI9341_SetBacklight(tft_ili9341_t *tft, bool on);

uint16_t TFT_ILI9341_GetWidth(const tft_ili9341_t *tft);
uint16_t TFT_ILI9341_GetHeight(const tft_ili9341_t *tft);

/* Low-level functions are exposed for legacy graphics/font libraries. */
tft_ili9341_status_t TFT_ILI9341_WriteCommand(tft_ili9341_t *tft,
                                               uint8_t command);
tft_ili9341_status_t TFT_ILI9341_WriteData(tft_ili9341_t *tft,
                                            const uint8_t *data,
                                            size_t length);
tft_ili9341_status_t TFT_ILI9341_SetAddressWindow(tft_ili9341_t *tft,
                                                  uint16_t x0,
                                                  uint16_t y0,
                                                  uint16_t x1,
                                                  uint16_t y1);

tft_ili9341_status_t TFT_ILI9341_DrawPixel(tft_ili9341_t *tft,
                                            int32_t x,
                                            int32_t y,
                                            uint16_t color);
tft_ili9341_status_t TFT_ILI9341_FillRect(tft_ili9341_t *tft,
                                           int32_t x,
                                           int32_t y,
                                           int32_t width,
                                           int32_t height,
                                           uint16_t color);
tft_ili9341_status_t TFT_ILI9341_FillScreen(tft_ili9341_t *tft,
                                             uint16_t color);
tft_ili9341_status_t TFT_ILI9341_DrawLine(tft_ili9341_t *tft,
                                           int32_t x0,
                                           int32_t y0,
                                           int32_t x1,
                                           int32_t y1,
                                           uint16_t color);
tft_ili9341_status_t TFT_ILI9341_DrawRect(tft_ili9341_t *tft,
                                           int32_t x,
                                           int32_t y,
                                           int32_t width,
                                           int32_t height,
                                           uint16_t color);
tft_ili9341_status_t TFT_ILI9341_DrawRGB565(tft_ili9341_t *tft,
                                             int32_t x,
                                             int32_t y,
                                             int32_t width,
                                             int32_t height,
                                             const uint16_t *pixels);

/*
 * Text and monochrome bitmap helpers.
 *
 * Built-in fonts cover printable ASCII 0x20 through 0x7E. Unsupported bytes
 * are drawn as '?'. The bitmap API is the extension point for selected
 * Chinese/custom glyphs: row-major, ceil(width / 8) bytes per row, LSB first.
 * Set transparent_background=true to leave zero bits unchanged.
 */
tft_ili9341_status_t TFT_ILI9341_GetFontMetrics(
    tft_ili9341_font_t font,
    uint8_t *width,
    uint8_t *height);
tft_ili9341_status_t TFT_ILI9341_DrawMonoBitmap(
    tft_ili9341_t *tft,
    int32_t x,
    int32_t y,
    uint8_t width,
    uint8_t height,
    const uint8_t *bitmap,
    size_t bitmap_size,
    uint16_t foreground,
    uint16_t background,
    bool transparent_background);
tft_ili9341_status_t TFT_ILI9341_DrawChar(
    tft_ili9341_t *tft,
    int32_t x,
    int32_t y,
    char character,
    tft_ili9341_font_t font,
    uint16_t foreground,
    uint16_t background,
    bool transparent_background);
tft_ili9341_status_t TFT_ILI9341_DrawString(
    tft_ili9341_t *tft,
    int32_t x,
    int32_t y,
    const char *text,
    tft_ili9341_font_t font,
    uint16_t foreground,
    uint16_t background,
    bool transparent_background,
    bool wrap);
tft_ili9341_status_t TFT_ILI9341_DrawInt32(
    tft_ili9341_t *tft,
    int32_t x,
    int32_t y,
    int32_t value,
    tft_ili9341_font_t font,
    uint16_t foreground,
    uint16_t background,
    bool transparent_background);
tft_ili9341_status_t TFT_ILI9341_DrawFloat(
    tft_ili9341_t *tft,
    int32_t x,
    int32_t y,
    float value,
    uint8_t decimal_places,
    tft_ili9341_font_t font,
    uint16_t foreground,
    uint16_t background,
    bool transparent_background);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */

signal_module_status_t SignalTFTILI9341_GetModuleStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_TFT_ILI9341_H */

