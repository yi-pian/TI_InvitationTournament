#ifndef SIGNAL_TFT_ST7789_H
#define SIGNAL_TFT_ST7789_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "signal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TFT_ST7789_NATIVE_WIDTH  (240U)
#define TFT_ST7789_NATIVE_HEIGHT (320U)

#define TFT_ST7789_BLACK   UINT16_C(0x0000)
#define TFT_ST7789_WHITE   UINT16_C(0xFFFF)
#define TFT_ST7789_RED     UINT16_C(0xF800)
#define TFT_ST7789_GREEN   UINT16_C(0x07E0)
#define TFT_ST7789_BLUE    UINT16_C(0x001F)
#define TFT_ST7789_CYAN    UINT16_C(0x07FF)
#define TFT_ST7789_MAGENTA UINT16_C(0xF81F)
#define TFT_ST7789_YELLOW  UINT16_C(0xFFE0)
#define TFT_ST7789_RGB565(red, green, blue) \
    ((uint16_t)((((uint16_t)(red) & UINT16_C(0xF8)) << 8U) | \
                (((uint16_t)(green) & UINT16_C(0xFC)) << 3U) | \
                ((uint16_t)(blue) >> 3U)))

typedef signal_result_t tft_st7789_status_t;
#define TFT_ST7789_OK                    SIGNAL_RESULT_OK
#define TFT_ST7789_ERROR_ARGUMENT        SIGNAL_RESULT_INVALID_ARGUMENT
#define TFT_ST7789_ERROR_IO              SIGNAL_RESULT_HARDWARE_ERROR
#define TFT_ST7789_ERROR_NOT_INITIALIZED SIGNAL_RESULT_NOT_INITIALIZED
#define TFT_ST7789_ERROR_RANGE           SIGNAL_RESULT_OUT_OF_RANGE

typedef enum {
    TFT_ST7789_ROTATION_0 = 0,
    TFT_ST7789_ROTATION_90,
    TFT_ST7789_ROTATION_180,
    TFT_ST7789_ROTATION_270
} tft_st7789_rotation_t;

typedef int (*tft_st7789_write_fn)(void *context,
    const uint8_t *data, size_t length);
typedef void (*tft_st7789_gpio_fn)(void *context, bool high);
typedef void (*tft_st7789_delay_fn)(void *context, uint32_t milliseconds);
typedef void (*tft_st7789_sync_fn)(void *context);

typedef struct {
    void *context;
    tft_st7789_write_fn write;
    tft_st7789_gpio_fn set_cs;       /* Optional when SPI owns CS. */
    tft_st7789_gpio_fn set_dc;
    tft_st7789_gpio_fn set_reset;    /* Optional when reset is tied high. */
    tft_st7789_gpio_fn set_backlight;
    tft_st7789_delay_fn delay_ms;
    tft_st7789_sync_fn lock;
    tft_st7789_sync_fn unlock;
    uint16_t x_offset;
    uint16_t y_offset;
} tft_st7789_config_t;

typedef struct {
    tft_st7789_config_t config;
    uint16_t width;
    uint16_t height;
    tft_st7789_rotation_t rotation;
    bool initialized;
} tft_st7789_t;

tft_st7789_status_t TFT_ST7789_Init(tft_st7789_t *tft,
    const tft_st7789_config_t *config, tft_st7789_rotation_t rotation);
tft_st7789_status_t TFT_ST7789_SetRotation(tft_st7789_t *tft,
    tft_st7789_rotation_t rotation);
void TFT_ST7789_SetBacklight(tft_st7789_t *tft, bool on);
uint16_t TFT_ST7789_GetWidth(const tft_st7789_t *tft);
uint16_t TFT_ST7789_GetHeight(const tft_st7789_t *tft);

tft_st7789_status_t TFT_ST7789_WriteCommand(tft_st7789_t *tft,
    uint8_t command);
tft_st7789_status_t TFT_ST7789_WriteData(tft_st7789_t *tft,
    const uint8_t *data, size_t length);
tft_st7789_status_t TFT_ST7789_SetAddressWindow(tft_st7789_t *tft,
    uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
tft_st7789_status_t TFT_ST7789_DrawPixel(tft_st7789_t *tft,
    int32_t x, int32_t y, uint16_t color);
tft_st7789_status_t TFT_ST7789_FillRect(tft_st7789_t *tft,
    int32_t x, int32_t y, int32_t width, int32_t height, uint16_t color);
tft_st7789_status_t TFT_ST7789_FillScreen(tft_st7789_t *tft,
    uint16_t color);
tft_st7789_status_t TFT_ST7789_DrawLine(tft_st7789_t *tft,
    int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color);
tft_st7789_status_t TFT_ST7789_DrawRect(tft_st7789_t *tft,
    int32_t x, int32_t y, int32_t width, int32_t height, uint16_t color);
tft_st7789_status_t TFT_ST7789_DrawRGB565(tft_st7789_t *tft,
    int32_t x, int32_t y, int32_t width, int32_t height,
    const uint16_t *pixels);
tft_st7789_status_t TFT_ST7789_DrawMonoBitmap(tft_st7789_t *tft,
    int32_t x, int32_t y, uint8_t width, uint8_t height,
    const uint8_t *bitmap, size_t bitmap_size,
    uint16_t foreground, uint16_t background, bool transparent_background);

signal_module_status_t SignalTFTST7789_GetModuleStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_TFT_ST7789_H */
