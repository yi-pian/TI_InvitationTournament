#include "ssd1306.h"

#include <string.h>

#include "ssd1306_font_6x8.inc"

#define SSD1306_CONTROL_COMMAND  (0x00U)
#define SSD1306_CONTROL_DATA     (0x40U)

static ssd1306_status_t SSD1306_Write(ssd1306_t *device,
    uint8_t control,
    const uint8_t *data,
    size_t count)
{
    if ((device == NULL) || (device->config.write == NULL) ||
        (data == NULL) || (count == 0U)) {
        return SSD1306_STATUS_BAD_ARGUMENT;
    }
    return device->config.write(device->config.io_context,
               control, data, count) ?
        SSD1306_STATUS_OK : SSD1306_STATUS_IO_ERROR;
}

static ssd1306_status_t SSD1306_WriteCommand(ssd1306_t *device,
    uint8_t command)
{
    return SSD1306_Write(
        device, SSD1306_CONTROL_COMMAND, &command, 1U);
}

static ssd1306_status_t SSD1306_WriteCommands(ssd1306_t *device,
    const uint8_t *commands,
    size_t count)
{
    return SSD1306_Write(
        device, SSD1306_CONTROL_COMMAND, commands, count);
}

static ssd1306_status_t SSD1306_SetRotationRaw(ssd1306_t *device,
    bool rotate_180)
{
    const uint8_t commands[2] = {
        rotate_180 ? 0xA0U : 0xA1U,
        rotate_180 ? 0xC0U : 0xC8U
    };
    ssd1306_status_t status = SSD1306_WriteCommands(
        device, commands, sizeof(commands));

    if (status == SSD1306_STATUS_OK) {
        device->config.rotate_180 = rotate_180;
    }
    return status;
}

ssd1306_status_t SSD1306_Init(ssd1306_t *device,
    const ssd1306_config_t *config)
{
    static const uint8_t init_before_rotation[] = {
        0xAEU,             /* display off */
        0xD5U, 0x80U,     /* display clock */
        0xA8U, 0x3FU,     /* 1/64 multiplex */
        0xD3U, 0x00U,     /* no display offset */
        0x40U,             /* start line 0 */
        0x8DU, 0x14U,     /* charge pump on */
        0x20U, 0x02U      /* page addressing mode */
    };
    static const uint8_t init_after_rotation[] = {
        0xDAU, 0x12U,     /* alternative COM pin configuration */
        0xD9U, 0xF1U,     /* pre-charge */
        0xDBU, 0x30U,     /* VCOMH */
        0xA4U,             /* resume RAM display */
        0xA6U              /* normal display */
    };
    ssd1306_status_t status;

    if ((device == NULL) || (config == NULL) ||
        (config->write == NULL)) {
        return SSD1306_STATUS_BAD_ARGUMENT;
    }

    device->config = *config;
    device->initialized = false;

    status = SSD1306_WriteCommands(device, init_before_rotation,
        sizeof(init_before_rotation));
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    status = SSD1306_SetRotationRaw(device, config->rotate_180);
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    status = SSD1306_WriteCommands(device, init_after_rotation,
        sizeof(init_after_rotation));
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    status = SSD1306_WriteCommand(device, 0x81U);
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    status = SSD1306_WriteCommand(device, config->contrast);
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    status = SSD1306_WriteCommand(device, 0xAFU);
    if (status == SSD1306_STATUS_OK) {
        device->initialized = true;
    }
    return status;
}

ssd1306_status_t SSD1306_SetDisplayOn(ssd1306_t *device, bool on)
{
    if (device == NULL) {
        return SSD1306_STATUS_BAD_ARGUMENT;
    }
    if (!device->initialized) {
        return SSD1306_STATUS_NOT_INITIALIZED;
    }
    return SSD1306_WriteCommand(device, on ? 0xAFU : 0xAEU);
}

ssd1306_status_t SSD1306_SetContrast(ssd1306_t *device,
    uint8_t contrast)
{
    const uint8_t commands[2] = {0x81U, contrast};
    ssd1306_status_t status;

    if (device == NULL) {
        return SSD1306_STATUS_BAD_ARGUMENT;
    }
    if (!device->initialized) {
        return SSD1306_STATUS_NOT_INITIALIZED;
    }
    status = SSD1306_WriteCommands(device, commands, sizeof(commands));
    if (status == SSD1306_STATUS_OK) {
        device->config.contrast = contrast;
    }
    return status;
}

ssd1306_status_t SSD1306_SetInverse(ssd1306_t *device, bool inverse)
{
    if (device == NULL) {
        return SSD1306_STATUS_BAD_ARGUMENT;
    }
    if (!device->initialized) {
        return SSD1306_STATUS_NOT_INITIALIZED;
    }
    return SSD1306_WriteCommand(device, inverse ? 0xA7U : 0xA6U);
}

ssd1306_status_t SSD1306_SetRotation(ssd1306_t *device,
    bool rotate_180)
{
    if (device == NULL) {
        return SSD1306_STATUS_BAD_ARGUMENT;
    }
    if (!device->initialized) {
        return SSD1306_STATUS_NOT_INITIALIZED;
    }
    return SSD1306_SetRotationRaw(device, rotate_180);
}

ssd1306_status_t SSD1306_Update(ssd1306_t *device,
    const uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE])
{
    uint8_t page;

    if ((device == NULL) || (framebuffer == NULL)) {
        return SSD1306_STATUS_BAD_ARGUMENT;
    }
    if (!device->initialized) {
        return SSD1306_STATUS_NOT_INITIALIZED;
    }

    for (page = 0U; page < SSD1306_PAGE_COUNT; ++page) {
        const uint8_t position[3] = {
            (uint8_t) (0xB0U + page), 0x00U, 0x10U
        };
        ssd1306_status_t status = SSD1306_WriteCommands(
            device, position, sizeof(position));
        if (status != SSD1306_STATUS_OK) {
            return status;
        }
        status = SSD1306_Write(device, SSD1306_CONTROL_DATA,
            &framebuffer[(size_t) page * SSD1306_WIDTH], SSD1306_WIDTH);
        if (status != SSD1306_STATUS_OK) {
            return status;
        }
    }
    return SSD1306_STATUS_OK;
}

uint16_t SSD1306_GetWidth(const ssd1306_t *device)
{
    return (device != NULL && device->initialized) ? SSD1306_WIDTH : 0U;
}

uint16_t SSD1306_GetHeight(const ssd1306_t *device)
{
    return (device != NULL && device->initialized) ? SSD1306_HEIGHT : 0U;
}

void SSD1306_ClearBuffer(
    uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE])
{
    if (framebuffer != NULL) {
        (void) memset(framebuffer, 0, SSD1306_FRAMEBUFFER_SIZE);
    }
}

bool SSD1306_DrawPixel(uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE],
    uint8_t x,
    uint8_t y,
    bool on)
{
    size_t index;
    uint8_t mask;

    if ((framebuffer == NULL) || (x >= SSD1306_WIDTH) ||
        (y >= SSD1306_HEIGHT)) {
        return false;
    }
    index = ((size_t) (y >> 3U) * SSD1306_WIDTH) + x;
    mask = (uint8_t) (1U << (y & 7U));
    if (on) {
        framebuffer[index] |= mask;
    } else {
        framebuffer[index] &= (uint8_t) ~mask;
    }
    return true;
}

void SSD1306_DrawLine(uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE],
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1,
    bool on)
{
    int16_t dx = (x1 >= x0) ? (int16_t) (x1 - x0) : (int16_t) (x0 - x1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t dy_abs = (y1 >= y0) ? (int16_t) (y1 - y0) :
        (int16_t) (y0 - y1);
    int16_t dy = (int16_t) -dy_abs;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t error = (int16_t) (dx + dy);

    if (framebuffer == NULL) {
        return;
    }
    for (;;) {
        if ((x0 >= 0) && (x0 < (int16_t) SSD1306_WIDTH) &&
            (y0 >= 0) && (y0 < (int16_t) SSD1306_HEIGHT)) {
            (void) SSD1306_DrawPixel(framebuffer,
                (uint8_t) x0, (uint8_t) y0, on);
        }
        if ((x0 == x1) && (y0 == y1)) {
            break;
        }
        {
            int16_t twice_error = (int16_t) (2 * error);
            if (twice_error >= dy) {
                error = (int16_t) (error + dy);
                x0 = (int16_t) (x0 + sx);
            }
            if (twice_error <= dx) {
                error = (int16_t) (error + dx);
                y0 = (int16_t) (y0 + sy);
            }
        }
    }
}

bool SSD1306_DrawChar6x8(
    uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE],
    uint8_t x,
    uint8_t y,
    char character,
    bool on)
{
    uint8_t column;
    uint8_t row;
    uint8_t glyph_index;

    if ((framebuffer == NULL) || (character < ' ') ||
        (character > '~') || ((uint16_t) x + 6U > SSD1306_WIDTH) ||
        ((uint16_t) y + 8U > SSD1306_HEIGHT)) {
        return false;
    }
    glyph_index = (uint8_t) ((uint8_t) character - (uint8_t) ' ');
    for (column = 0U; column < 6U; ++column) {
        uint8_t bits = g_ssd1306_font_6x8[glyph_index][column];
        for (row = 0U; row < 8U; ++row) {
            bool pixel_on = ((bits >> row) & 1U) != 0U;
            (void) SSD1306_DrawPixel(framebuffer,
                (uint8_t) (x + column), (uint8_t) (y + row),
                pixel_on ? on : !on);
        }
    }
    return true;
}

size_t SSD1306_DrawString6x8(
    uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE],
    uint8_t x,
    uint8_t y,
    const char *text,
    bool on)
{
    size_t count = 0U;

    if ((framebuffer == NULL) || (text == NULL)) {
        return 0U;
    }
    while ((*text != '\0') &&
        SSD1306_DrawChar6x8(framebuffer, x, y, *text, on)) {
        ++count;
        ++text;
        x = (uint8_t) (x + 6U);
    }
    return count;
}

bool SSD1306_DrawRect(uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE],
    int16_t x, int16_t y, int16_t width, int16_t height, bool on)
{
    if (width <= 0 || height <= 0) return false;
    SSD1306_DrawLine(framebuffer, x, y, (int16_t)(x + width - 1), y, on);
    SSD1306_DrawLine(framebuffer, x, (int16_t)(y + height - 1),
        (int16_t)(x + width - 1), (int16_t)(y + height - 1), on);
    SSD1306_DrawLine(framebuffer, x, y, x, (int16_t)(y + height - 1), on);
    SSD1306_DrawLine(framebuffer, (int16_t)(x + width - 1), y,
        (int16_t)(x + width - 1), (int16_t)(y + height - 1), on);
    return framebuffer != NULL;
}

bool SSD1306_DrawBitmap(uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE],
    int16_t x, int16_t y, uint8_t width, uint8_t height,
    const uint8_t *bitmap, size_t bitmap_size, bool on,
    bool transparent_background)
{
    uint8_t bytes_per_row;
    uint8_t row;
    uint8_t column;
    if (framebuffer == NULL || bitmap == NULL || width == 0U || height == 0U)
        return false;
    bytes_per_row = (uint8_t)((width + 7U) / 8U);
    if (bitmap_size < (size_t)bytes_per_row * height) return false;
    for (row = 0U; row < height; ++row) {
        for (column = 0U; column < width; ++column) {
            bool bit = (bitmap[(size_t)row * bytes_per_row + (column >> 3U)] &
                (uint8_t)(1U << (column & 7U))) != 0U;
            if (!bit && transparent_background) continue;
            if (x + column >= 0 && y + row >= 0 &&
                x + column < (int16_t)SSD1306_WIDTH &&
                y + row < (int16_t)SSD1306_HEIGHT) {
                (void)SSD1306_DrawPixel(framebuffer, (uint8_t)(x + column),
                    (uint8_t)(y + row), bit ? on : !on);
            }
        }
    }
    return true;
}

signal_module_status_t SignalSSD1306_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
