#ifndef MSPM0_EXTERNAL_SSD1306_H
#define MSPM0_EXTERNAL_SSD1306_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "signal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SSD1306_WIDTH             (128U)
#define SSD1306_HEIGHT            (64U)
#define SSD1306_PAGE_COUNT        (SSD1306_HEIGHT / 8U)
#define SSD1306_FRAMEBUFFER_SIZE  (SSD1306_WIDTH * SSD1306_PAGE_COUNT)

#define SSD1306_I2C_ADDRESS_DEFAULT    (0x3CU)
#define SSD1306_I2C_ADDRESS_ALTERNATE  (0x3DU)

typedef enum {
    SSD1306_STATUS_OK = 0,
    SSD1306_STATUS_BAD_ARGUMENT,
    SSD1306_STATUS_IO_ERROR,
    SSD1306_STATUS_NOT_INITIALIZED,
    SSD1306_STATUS_OUT_OF_RANGE
} ssd1306_status_t;

typedef bool (*ssd1306_write_fn)(void *context,
    uint8_t control,
    const uint8_t *data,
    size_t count);

typedef struct {
    void *io_context;
    ssd1306_write_fn write;
    uint8_t contrast;
    bool rotate_180;
} ssd1306_config_t;

typedef struct {
    ssd1306_config_t config;
    bool initialized;
} ssd1306_t;

ssd1306_status_t SSD1306_Init(ssd1306_t *device,
    const ssd1306_config_t *config);
ssd1306_status_t SSD1306_SetDisplayOn(ssd1306_t *device, bool on);
ssd1306_status_t SSD1306_SetContrast(ssd1306_t *device,
    uint8_t contrast);
ssd1306_status_t SSD1306_SetInverse(ssd1306_t *device, bool inverse);
ssd1306_status_t SSD1306_SetRotation(ssd1306_t *device,
    bool rotate_180);
ssd1306_status_t SSD1306_Update(ssd1306_t *device,
    const uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE]);

uint16_t SSD1306_GetWidth(const ssd1306_t *device);
uint16_t SSD1306_GetHeight(const ssd1306_t *device);

void SSD1306_ClearBuffer(
    uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE]);
bool SSD1306_DrawPixel(uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE],
    uint8_t x,
    uint8_t y,
    bool on);
void SSD1306_DrawLine(uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE],
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1,
    bool on);
bool SSD1306_DrawChar6x8(
    uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE],
    uint8_t x,
    uint8_t y,
    char character,
    bool on);
size_t SSD1306_DrawString6x8(
    uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE],
    uint8_t x,
    uint8_t y,
    const char *text,
    bool on);

bool SSD1306_DrawRect(uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE],
    int16_t x, int16_t y, int16_t width, int16_t height, bool on);
bool SSD1306_DrawBitmap(uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE],
    int16_t x, int16_t y, uint8_t width, uint8_t height,
    const uint8_t *bitmap, size_t bitmap_size, bool on,
    bool transparent_background);

signal_module_status_t SignalSSD1306_GetModuleStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* MSPM0_EXTERNAL_SSD1306_H */
