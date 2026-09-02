#include "signal_tft_ili9341.h"

#include <string.h>

#define ILI9341_CMD_SWRESET  UINT8_C(0x01)
#define ILI9341_CMD_SLPOUT   UINT8_C(0x11)
#define ILI9341_CMD_DISPON   UINT8_C(0x29)
#define ILI9341_CMD_CASET    UINT8_C(0x2A)
#define ILI9341_CMD_PASET    UINT8_C(0x2B)
#define ILI9341_CMD_RAMWR    UINT8_C(0x2C)
#define ILI9341_CMD_MADCTL   UINT8_C(0x36)
#define ILI9341_CMD_PIXFMT   UINT8_C(0x3A)

#define INIT_DELAY_FLAG       UINT8_C(0x80)
#define PIXEL_BUFFER_COUNT    64U
#define TFT_ASCII_FIRST       UINT8_C(0x20)
#define TFT_ASCII_LAST        UINT8_C(0x7E)

#include "signal_tft_ili9341_font_data.inc"

/* 16x16 sample glyphs "电" and "子" from the supplied lcdfont.h. */
const uint8_t TFT_ILI9341_GLYPH_CN_DIAN_16X16[
    TFT_ILI9341_GLYPH_16X16_BYTES] = {
    0x80U, 0x00U, 0x80U, 0x00U, 0x80U, 0x00U, 0xFCU, 0x1FU,
    0x84U, 0x10U, 0x84U, 0x10U, 0x84U, 0x10U, 0xFCU, 0x1FU,
    0x84U, 0x10U, 0x84U, 0x10U, 0x84U, 0x10U, 0xFCU, 0x1FU,
    0x84U, 0x50U, 0x80U, 0x40U, 0x80U, 0x40U, 0x00U, 0x7FU
};

const uint8_t TFT_ILI9341_GLYPH_CN_ZI_16X16[
    TFT_ILI9341_GLYPH_16X16_BYTES] = {
    0x00U, 0x00U, 0xFEU, 0x1FU, 0x00U, 0x08U, 0x00U, 0x04U,
    0x00U, 0x02U, 0x80U, 0x01U, 0x80U, 0x00U, 0xFFU, 0x7FU,
    0x80U, 0x00U, 0x80U, 0x00U, 0x80U, 0x00U, 0x80U, 0x00U,
    0x80U, 0x00U, 0x80U, 0x00U, 0xA0U, 0x00U, 0x40U, 0x00U
};

typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t bytes_per_row;
    size_t bytes_per_glyph;
    const uint8_t *glyphs;
} tft_font_descriptor_t;

/* Compact format: command, argument-count|delay-flag, arguments, delay-ms. */
static const uint8_t g_init_commands[] = {
    ILI9341_CMD_SWRESET, INIT_DELAY_FLAG, 150U,
    ILI9341_CMD_SLPOUT, INIT_DELAY_FLAG, 120U,
    0xCFU, 3U, 0x00U, 0xC1U, 0x30U,
    0xEDU, 4U, 0x64U, 0x03U, 0x12U, 0x81U,
    0xE8U, 3U, 0x85U, 0x00U, 0x79U,
    0xCBU, 5U, 0x39U, 0x2CU, 0x00U, 0x34U, 0x02U,
    0xF7U, 1U, 0x20U,
    0xEAU, 2U, 0x00U, 0x00U,
    0xC0U, 1U, 0x1DU,
    0xC1U, 1U, 0x12U,
    0xC5U, 2U, 0x33U, 0x3FU,
    0xC7U, 1U, 0x92U,
    ILI9341_CMD_PIXFMT, 1U, 0x55U,
    0xB1U, 2U, 0x00U, 0x12U,
    0xB6U, 2U, 0x0AU, 0xA2U,
    0xF2U, 1U, 0x00U,
    0x26U, 1U, 0x01U,
    0xE0U, 15U, 0x0FU, 0x22U, 0x1CU, 0x1BU, 0x08U,
          0x0FU, 0x48U, 0xB8U, 0x34U, 0x05U, 0x0CU, 0x09U,
          0x0FU, 0x07U, 0x00U,
    0xE1U, 15U, 0x00U, 0x23U, 0x24U, 0x07U, 0x10U,
          0x07U, 0x38U, 0x47U, 0x4BU, 0x0AU, 0x13U, 0x06U,
          0x30U, 0x38U, 0x0FU,
    ILI9341_CMD_DISPON, INIT_DELAY_FLAG, 20U
};

static bool tft_is_valid(const tft_ili9341_t *tft)
{
    return (tft != NULL) && (tft->config.write != NULL) &&
           (tft->config.set_dc != NULL);
}

static const tft_font_descriptor_t *tft_get_font(
    tft_ili9341_font_t font)
{
    static const tft_font_descriptor_t fonts[] = {
        {6U, 12U, 1U, 12U, &g_font_ascii_6x12[0][0]},
        {8U, 16U, 1U, 16U, &g_font_ascii_8x16[0][0]},
        {12U, 24U, 2U, 48U, &g_font_ascii_12x24[0][0]},
        {16U, 32U, 2U, 64U, &g_font_ascii_16x32[0][0]}
    };

    if ((uint32_t)font >= (sizeof(fonts) / sizeof(fonts[0]))) {
        return NULL;
    }
    return &fonts[(uint32_t)font];
}

static void tft_set_cs(tft_ili9341_t *tft, bool high)
{
    if (tft->config.set_cs != NULL) {
        tft->config.set_cs(tft->config.context, high);
    }
}

static void tft_lock(tft_ili9341_t *tft)
{
    if (tft->config.lock != NULL) {
        tft->config.lock(tft->config.context);
    }
}

static void tft_unlock(tft_ili9341_t *tft)
{
    if (tft->config.unlock != NULL) {
        tft->config.unlock(tft->config.context);
    }
}

static tft_ili9341_status_t tft_transaction_unlocked(tft_ili9341_t *tft,
                                                      bool data_mode,
                                                      const uint8_t *data,
                                                      size_t length)
{
    int result;

    if (!tft_is_valid(tft) || ((data == NULL) && (length != 0U))) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    if (length == 0U) {
        return TFT_ILI9341_OK;
    }

    tft->config.set_dc(tft->config.context, data_mode);
    tft_set_cs(tft, false);
    result = tft->config.write(tft->config.context, data, length);
    tft_set_cs(tft, true);

    return (result == 0) ? TFT_ILI9341_OK : TFT_ILI9341_ERROR_IO;
}

static tft_ili9341_status_t tft_transaction(tft_ili9341_t *tft,
                                             bool data_mode,
                                             const uint8_t *data,
                                             size_t length)
{
    tft_ili9341_status_t status;

    if (!tft_is_valid(tft)) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    tft_lock(tft);
    status = tft_transaction_unlocked(tft, data_mode, data, length);
    tft_unlock(tft);
    return status;
}

static tft_ili9341_status_t tft_command_data_unlocked(
    tft_ili9341_t *tft,
    uint8_t command,
    const uint8_t *data,
    size_t length)
{
    tft_ili9341_status_t status;

    status = tft_transaction_unlocked(tft, false, &command, 1U);
    if ((status == TFT_ILI9341_OK) && (length != 0U)) {
        status = tft_transaction_unlocked(tft, true, data, length);
    }
    return status;
}

static tft_ili9341_status_t tft_command_data(tft_ili9341_t *tft,
                                              uint8_t command,
                                              const uint8_t *data,
                                              size_t length)
{
    tft_ili9341_status_t status;

    if (!tft_is_valid(tft)) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    tft_lock(tft);
    status = tft_command_data_unlocked(tft, command, data, length);
    tft_unlock(tft);
    return status;
}

tft_ili9341_status_t TFT_ILI9341_WriteCommand(tft_ili9341_t *tft,
                                               uint8_t command)
{
    return tft_transaction(tft, false, &command, 1U);
}

tft_ili9341_status_t TFT_ILI9341_WriteData(tft_ili9341_t *tft,
                                            const uint8_t *data,
                                            size_t length)
{
    return tft_transaction(tft, true, data, length);
}

tft_ili9341_status_t TFT_ILI9341_Init(tft_ili9341_t *tft,
                                      const tft_ili9341_config_t *config,
                                      tft_ili9341_rotation_t rotation)
{
    size_t offset = 0U;

    if ((tft == NULL) || (config == NULL) || (config->write == NULL) ||
        (config->set_dc == NULL) ||
        (config->delay_ms == NULL) ||
        ((config->lock == NULL) != (config->unlock == NULL)) ||
        (rotation > TFT_ILI9341_ROTATION_90)) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }

    (void)memset(tft, 0, sizeof(*tft));
    tft->config = *config;
    tft->width = TFT_ILI9341_NATIVE_WIDTH;
    tft->height = TFT_ILI9341_NATIVE_HEIGHT;

    if (config->set_cs != NULL) {
        config->set_cs(config->context, true);
    }
    config->set_dc(config->context, true);
    if (config->set_backlight != NULL) {
        config->set_backlight(config->context, false);
    }
    if (config->set_reset != NULL) {
        config->set_reset(config->context, true);
        config->delay_ms(config->context, 5U);
        config->set_reset(config->context, false);
        config->delay_ms(config->context, 20U);
        config->set_reset(config->context, true);
        config->delay_ms(config->context, 120U);
    }

    while (offset < sizeof(g_init_commands)) {
        uint8_t command = g_init_commands[offset++];
        uint8_t descriptor = g_init_commands[offset++];
        size_t argument_count = (size_t)(descriptor & ~INIT_DELAY_FLAG);
        tft_ili9341_status_t status;

        status = tft_command_data(tft, command,
                                  &g_init_commands[offset], argument_count);
        if (status != TFT_ILI9341_OK) {
            return status;
        }
        offset += argument_count;
        if ((descriptor & INIT_DELAY_FLAG) != 0U) {
            config->delay_ms(config->context, g_init_commands[offset++]);
        }
    }

    tft->initialized = true;
    if (TFT_ILI9341_SetRotation(tft, rotation) != TFT_ILI9341_OK) {
        tft->initialized = false;
        return TFT_ILI9341_ERROR_IO;
    }
    TFT_ILI9341_SetBacklight(tft, true);
    return TFT_ILI9341_OK;
}

tft_ili9341_status_t TFT_ILI9341_SetRotation(
    tft_ili9341_t *tft,
    tft_ili9341_rotation_t rotation)
{
    static const uint8_t madctl[] = {0x08U, 0xC8U, 0x78U, 0xA8U};
    tft_ili9341_status_t status;

    if (!tft_is_valid(tft) || (rotation > TFT_ILI9341_ROTATION_90)) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    if (!tft->initialized) {
        return TFT_ILI9341_ERROR_NOT_INITIALIZED;
    }

    status = tft_command_data(tft, ILI9341_CMD_MADCTL,
                              &madctl[(uint8_t)rotation], 1U);
    if (status == TFT_ILI9341_OK) {
        tft->rotation = rotation;
        if ((rotation == TFT_ILI9341_ROTATION_0) ||
            (rotation == TFT_ILI9341_ROTATION_180)) {
            tft->width = TFT_ILI9341_NATIVE_WIDTH;
            tft->height = TFT_ILI9341_NATIVE_HEIGHT;
        } else {
            tft->width = TFT_ILI9341_NATIVE_HEIGHT;
            tft->height = TFT_ILI9341_NATIVE_WIDTH;
        }
    }
    return status;
}

void TFT_ILI9341_SetBacklight(tft_ili9341_t *tft, bool on)
{
    if ((tft != NULL) && (tft->config.set_backlight != NULL)) {
        tft->config.set_backlight(tft->config.context, on);
    }
}

uint16_t TFT_ILI9341_GetWidth(const tft_ili9341_t *tft)
{
    return (tft != NULL) ? tft->width : 0U;
}

uint16_t TFT_ILI9341_GetHeight(const tft_ili9341_t *tft)
{
    return (tft != NULL) ? tft->height : 0U;
}

static tft_ili9341_status_t tft_set_address_window_unlocked(
    tft_ili9341_t *tft,
    uint16_t x0,
    uint16_t y0,
    uint16_t x1,
    uint16_t y1)
{
    uint8_t coordinates[4];
    tft_ili9341_status_t status;

    if (!tft_is_valid(tft) || !tft->initialized) {
        return (tft == NULL || !tft_is_valid(tft))
                   ? TFT_ILI9341_ERROR_ARGUMENT
                   : TFT_ILI9341_ERROR_NOT_INITIALIZED;
    }
    if ((x0 > x1) || (y0 > y1) || (x1 >= tft->width) ||
        (y1 >= tft->height)) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }

    coordinates[0] = (uint8_t)(x0 >> 8U);
    coordinates[1] = (uint8_t)x0;
    coordinates[2] = (uint8_t)(x1 >> 8U);
    coordinates[3] = (uint8_t)x1;
    status = tft_command_data_unlocked(tft, ILI9341_CMD_CASET, coordinates,
                                       sizeof(coordinates));
    if (status != TFT_ILI9341_OK) {
        return status;
    }

    coordinates[0] = (uint8_t)(y0 >> 8U);
    coordinates[1] = (uint8_t)y0;
    coordinates[2] = (uint8_t)(y1 >> 8U);
    coordinates[3] = (uint8_t)y1;
    status = tft_command_data_unlocked(tft, ILI9341_CMD_PASET, coordinates,
                                       sizeof(coordinates));
    if (status != TFT_ILI9341_OK) {
        return status;
    }
    return tft_transaction_unlocked(tft, false, &((uint8_t){ILI9341_CMD_RAMWR}),
                                    1U);
}

tft_ili9341_status_t TFT_ILI9341_SetAddressWindow(tft_ili9341_t *tft,
                                                  uint16_t x0,
                                                  uint16_t y0,
                                                  uint16_t x1,
                                                  uint16_t y1)
{
    tft_ili9341_status_t status;

    if (!tft_is_valid(tft)) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    tft_lock(tft);
    status = tft_set_address_window_unlocked(tft, x0, y0, x1, y1);
    tft_unlock(tft);
    return status;
}

static tft_ili9341_status_t tft_write_repeated_color(tft_ili9341_t *tft,
                                                      uint16_t color,
                                                      uint32_t pixel_count)
{
    uint8_t buffer[PIXEL_BUFFER_COUNT * 2U];
    uint32_t i;

    for (i = 0U; i < PIXEL_BUFFER_COUNT; ++i) {
        buffer[i * 2U] = (uint8_t)(color >> 8U);
        buffer[i * 2U + 1U] = (uint8_t)color;
    }
    while (pixel_count != 0U) {
        uint32_t count = (pixel_count > PIXEL_BUFFER_COUNT)
                             ? PIXEL_BUFFER_COUNT
                             : pixel_count;
        tft_ili9341_status_t status = tft_transaction_unlocked(
            tft, true, buffer, (size_t)count * 2U);
        if (status != TFT_ILI9341_OK) {
            return status;
        }
        pixel_count -= count;
    }
    return TFT_ILI9341_OK;
}

static tft_ili9341_status_t tft_fill_rect_unlocked(tft_ili9341_t *tft,
                                                    int32_t x,
                                                    int32_t y,
                                                    int32_t width,
                                                    int32_t height,
                                                    uint16_t color)
{
    int32_t x1;
    int32_t y1;
    tft_ili9341_status_t status;

    if ((tft == NULL) || !tft->initialized) {
        return (tft == NULL) ? TFT_ILI9341_ERROR_ARGUMENT
                             : TFT_ILI9341_ERROR_NOT_INITIALIZED;
    }
    if ((width <= 0) || (height <= 0)) {
        return TFT_ILI9341_OK;
    }

    x1 = x + width;
    y1 = y + height;
    if ((x >= (int32_t)tft->width) || (y >= (int32_t)tft->height) ||
        (x1 <= 0) || (y1 <= 0)) {
        return TFT_ILI9341_OK;
    }
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (x1 > (int32_t)tft->width) {
        x1 = (int32_t)tft->width;
    }
    if (y1 > (int32_t)tft->height) {
        y1 = (int32_t)tft->height;
    }

    status = tft_set_address_window_unlocked(tft, (uint16_t)x, (uint16_t)y,
                                             (uint16_t)(x1 - 1),
                                             (uint16_t)(y1 - 1));
    if (status != TFT_ILI9341_OK) {
        return status;
    }
    return tft_write_repeated_color(
        tft, color, (uint32_t)(x1 - x) * (uint32_t)(y1 - y));
}

tft_ili9341_status_t TFT_ILI9341_FillRect(tft_ili9341_t *tft,
                                           int32_t x,
                                           int32_t y,
                                           int32_t width,
                                           int32_t height,
                                           uint16_t color)
{
    tft_ili9341_status_t status;

    if (!tft_is_valid(tft)) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    tft_lock(tft);
    status = tft_fill_rect_unlocked(tft, x, y, width, height, color);
    tft_unlock(tft);
    return status;
}

tft_ili9341_status_t TFT_ILI9341_FillScreen(tft_ili9341_t *tft,
                                             uint16_t color)
{
    if (tft == NULL) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    return TFT_ILI9341_FillRect(tft, 0, 0, tft->width, tft->height, color);
}

tft_ili9341_status_t TFT_ILI9341_DrawPixel(tft_ili9341_t *tft,
                                            int32_t x,
                                            int32_t y,
                                            uint16_t color)
{
    if ((tft == NULL) || !tft->initialized) {
        return (tft == NULL) ? TFT_ILI9341_ERROR_ARGUMENT
                             : TFT_ILI9341_ERROR_NOT_INITIALIZED;
    }
    if ((x < 0) || (y < 0) || (x >= (int32_t)tft->width) ||
        (y >= (int32_t)tft->height)) {
        return TFT_ILI9341_OK;
    }
    return TFT_ILI9341_FillRect(tft, x, y, 1, 1, color);
}

tft_ili9341_status_t TFT_ILI9341_DrawLine(tft_ili9341_t *tft,
                                           int32_t x0,
                                           int32_t y0,
                                           int32_t x1,
                                           int32_t y1,
                                           uint16_t color)
{
    int32_t dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = (y0 < y1) ? (y0 - y1) : (y1 - y0);
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t error = dx + dy;

    if ((tft == NULL) || !tft->initialized) {
        return (tft == NULL) ? TFT_ILI9341_ERROR_ARGUMENT
                             : TFT_ILI9341_ERROR_NOT_INITIALIZED;
    }
    tft_lock(tft);
    for (;;) {
        tft_ili9341_status_t status = tft_fill_rect_unlocked(
            tft, x0, y0, 1, 1, color);
        int32_t doubled_error;

        if (status != TFT_ILI9341_OK) {
            tft_unlock(tft);
            return status;
        }
        if ((x0 == x1) && (y0 == y1)) {
            break;
        }
        doubled_error = 2 * error;
        if (doubled_error >= dy) {
            error += dy;
            x0 += sx;
        }
        if (doubled_error <= dx) {
            error += dx;
            y0 += sy;
        }
    }
    tft_unlock(tft);
    return TFT_ILI9341_OK;
}

tft_ili9341_status_t TFT_ILI9341_DrawRect(tft_ili9341_t *tft,
                                           int32_t x,
                                           int32_t y,
                                           int32_t width,
                                           int32_t height,
                                           uint16_t color)
{
    tft_ili9341_status_t status;

    if ((width <= 0) || (height <= 0)) {
        return TFT_ILI9341_OK;
    }
    if (!tft_is_valid(tft)) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    tft_lock(tft);
    status = tft_fill_rect_unlocked(tft, x, y, width, 1, color);
    if (status == TFT_ILI9341_OK) {
        status = tft_fill_rect_unlocked(tft, x, y + height - 1, width, 1,
                                        color);
    }
    if (status == TFT_ILI9341_OK) {
        status = tft_fill_rect_unlocked(tft, x, y, 1, height, color);
    }
    if (status == TFT_ILI9341_OK) {
        status = tft_fill_rect_unlocked(tft, x + width - 1, y, 1, height,
                                        color);
    }
    tft_unlock(tft);
    return status;
}

tft_ili9341_status_t TFT_ILI9341_DrawRGB565(tft_ili9341_t *tft,
                                             int32_t x,
                                             int32_t y,
                                             int32_t width,
                                             int32_t height,
                                             const uint16_t *pixels)
{
    uint8_t buffer[PIXEL_BUFFER_COUNT * 2U];
    int32_t source_x = 0;
    int32_t source_y = 0;
    int32_t clipped_width;
    int32_t clipped_height;
    int32_t row;
    tft_ili9341_status_t status;

    if ((tft == NULL) || (pixels == NULL) || (width <= 0) || (height <= 0)) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    if (!tft->initialized) {
        return TFT_ILI9341_ERROR_NOT_INITIALIZED;
    }
    if ((x >= (int32_t)tft->width) || (y >= (int32_t)tft->height) ||
        ((x + width) <= 0) || ((y + height) <= 0)) {
        return TFT_ILI9341_OK;
    }

    clipped_width = width;
    clipped_height = height;
    if (x < 0) {
        source_x = -x;
        clipped_width += x;
        x = 0;
    }
    if (y < 0) {
        source_y = -y;
        clipped_height += y;
        y = 0;
    }
    if ((x + clipped_width) > (int32_t)tft->width) {
        clipped_width = (int32_t)tft->width - x;
    }
    if ((y + clipped_height) > (int32_t)tft->height) {
        clipped_height = (int32_t)tft->height - y;
    }

    tft_lock(tft);
    status = tft_set_address_window_unlocked(
        tft, (uint16_t)x, (uint16_t)y,
        (uint16_t)(x + clipped_width - 1),
        (uint16_t)(y + clipped_height - 1));
    if (status != TFT_ILI9341_OK) {
        tft_unlock(tft);
        return status;
    }

    for (row = 0; row < clipped_height; ++row) {
        int32_t column = 0;
        const uint16_t *source = pixels +
            (size_t)(source_y + row) * (size_t)width + (size_t)source_x;

        while (column < clipped_width) {
            int32_t count = clipped_width - column;
            int32_t i;
            if (count > (int32_t)PIXEL_BUFFER_COUNT) {
                count = (int32_t)PIXEL_BUFFER_COUNT;
            }
            for (i = 0; i < count; ++i) {
                uint16_t color = source[column + i];
                buffer[(size_t)i * 2U] = (uint8_t)(color >> 8U);
                buffer[(size_t)i * 2U + 1U] = (uint8_t)color;
            }
            status = tft_transaction_unlocked(tft, true, buffer,
                                              (size_t)count * 2U);
            if (status != TFT_ILI9341_OK) {
                tft_unlock(tft);
                return status;
            }
            column += count;
        }
    }
    tft_unlock(tft);
    return TFT_ILI9341_OK;
}

tft_ili9341_status_t TFT_ILI9341_GetFontMetrics(
    tft_ili9341_font_t font,
    uint8_t *width,
    uint8_t *height)
{
    const tft_font_descriptor_t *descriptor = tft_get_font(font);

    if ((descriptor == NULL) || (width == NULL) || (height == NULL)) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    *width = descriptor->width;
    *height = descriptor->height;
    return TFT_ILI9341_OK;
}

static bool tft_bitmap_pixel(const uint8_t *bitmap,
                             size_t bytes_per_row,
                             int32_t x,
                             int32_t y)
{
    size_t byte_index = (size_t)y * bytes_per_row + (size_t)x / 8U;
    uint8_t bit = (uint8_t)((uint32_t)x & UINT32_C(7));

    return (bitmap[byte_index] & (uint8_t)(UINT8_C(1) << bit)) != 0U;
}

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
    bool transparent_background)
{
    uint8_t buffer[PIXEL_BUFFER_COUNT * 2U];
    size_t bytes_per_row;
    size_t required_size;
    int32_t source_x = 0;
    int32_t source_y = 0;
    int32_t clipped_width;
    int32_t clipped_height;
    int32_t row;
    tft_ili9341_status_t status = TFT_ILI9341_OK;

    if (!tft_is_valid(tft) || (bitmap == NULL) ||
        (width == 0U) || (height == 0U)) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    if (!tft->initialized) {
        return TFT_ILI9341_ERROR_NOT_INITIALIZED;
    }

    bytes_per_row = ((size_t)width + 7U) / 8U;
    required_size = bytes_per_row * (size_t)height;
    if (bitmap_size < required_size) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    if ((x >= (int32_t)tft->width) || (y >= (int32_t)tft->height) ||
        ((x + (int32_t)width) <= 0) || ((y + (int32_t)height) <= 0)) {
        return TFT_ILI9341_OK;
    }

    clipped_width = (int32_t)width;
    clipped_height = (int32_t)height;
    if (x < 0) {
        source_x = -x;
        clipped_width -= source_x;
        x = 0;
    }
    if (y < 0) {
        source_y = -y;
        clipped_height -= source_y;
        y = 0;
    }
    if ((x + clipped_width) > (int32_t)tft->width) {
        clipped_width = (int32_t)tft->width - x;
    }
    if ((y + clipped_height) > (int32_t)tft->height) {
        clipped_height = (int32_t)tft->height - y;
    }

    tft_lock(tft);
    if (transparent_background) {
        for (row = 0; row < clipped_height; ++row) {
            int32_t column = 0;
            int32_t bitmap_y = source_y + row;

            while (column < clipped_width) {
                int32_t run_start;

                while ((column < clipped_width) &&
                       !tft_bitmap_pixel(bitmap, bytes_per_row,
                                         source_x + column, bitmap_y)) {
                    ++column;
                }
                if (column >= clipped_width) {
                    break;
                }
                run_start = column;
                while ((column < clipped_width) &&
                       tft_bitmap_pixel(bitmap, bytes_per_row,
                                        source_x + column, bitmap_y)) {
                    ++column;
                }
                status = tft_fill_rect_unlocked(tft, x + run_start, y + row,
                                                column - run_start, 1,
                                                foreground);
                if (status != TFT_ILI9341_OK) {
                    tft_unlock(tft);
                    return status;
                }
            }
        }
    } else {
        size_t buffered_pixels = 0U;

        status = tft_set_address_window_unlocked(
            tft, (uint16_t)x, (uint16_t)y,
            (uint16_t)(x + clipped_width - 1),
            (uint16_t)(y + clipped_height - 1));
        if (status != TFT_ILI9341_OK) {
            tft_unlock(tft);
            return status;
        }

        for (row = 0; row < clipped_height; ++row) {
            int32_t column;
            int32_t bitmap_y = source_y + row;

            for (column = 0; column < clipped_width; ++column) {
                uint16_t color = tft_bitmap_pixel(
                    bitmap, bytes_per_row, source_x + column, bitmap_y)
                                     ? foreground
                                     : background;
                buffer[buffered_pixels * 2U] = (uint8_t)(color >> 8U);
                buffer[buffered_pixels * 2U + 1U] = (uint8_t)color;
                ++buffered_pixels;

                if (buffered_pixels == PIXEL_BUFFER_COUNT) {
                    status = tft_transaction_unlocked(
                        tft, true, buffer, sizeof(buffer));
                    if (status != TFT_ILI9341_OK) {
                        tft_unlock(tft);
                        return status;
                    }
                    buffered_pixels = 0U;
                }
            }
        }
        if (buffered_pixels != 0U) {
            status = tft_transaction_unlocked(
                tft, true, buffer, buffered_pixels * 2U);
        }
    }
    tft_unlock(tft);
    return status;
}

tft_ili9341_status_t TFT_ILI9341_DrawChar(
    tft_ili9341_t *tft,
    int32_t x,
    int32_t y,
    char character,
    tft_ili9341_font_t font,
    uint16_t foreground,
    uint16_t background,
    bool transparent_background)
{
    const tft_font_descriptor_t *descriptor = tft_get_font(font);
    uint8_t code = (uint8_t)character;
    size_t glyph_index;
    const uint8_t *glyph;

    if (descriptor == NULL) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    if ((code < TFT_ASCII_FIRST) || (code > TFT_ASCII_LAST)) {
        code = (uint8_t)'?';
    }
    glyph_index = (size_t)(code - TFT_ASCII_FIRST);
    glyph = descriptor->glyphs + glyph_index * descriptor->bytes_per_glyph;

    return TFT_ILI9341_DrawMonoBitmap(
        tft, x, y, descriptor->width, descriptor->height,
        glyph, descriptor->bytes_per_glyph,
        foreground, background, transparent_background);
}

tft_ili9341_status_t TFT_ILI9341_DrawString(
    tft_ili9341_t *tft,
    int32_t x,
    int32_t y,
    const char *text,
    tft_ili9341_font_t font,
    uint16_t foreground,
    uint16_t background,
    bool transparent_background,
    bool wrap)
{
    const tft_font_descriptor_t *descriptor = tft_get_font(font);
    int32_t cursor_x = x;
    int32_t cursor_y = y;

    if ((tft == NULL) || (text == NULL) || (descriptor == NULL)) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }

    while (*text != '\0') {
        tft_ili9341_status_t status;

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
                     (int32_t)tft->width) && (cursor_x != x)) {
            cursor_x = x;
            cursor_y += descriptor->height;
        }
        status = TFT_ILI9341_DrawChar(
            tft, cursor_x, cursor_y, *text, font,
            foreground, background, transparent_background);
        if (status != TFT_ILI9341_OK) {
            return status;
        }
        cursor_x += descriptor->width;
        ++text;
    }
    return TFT_ILI9341_OK;
}

static size_t tft_append_uint32(char *buffer,
                                size_t position,
                                uint32_t value,
                                uint8_t minimum_digits)
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

tft_ili9341_status_t TFT_ILI9341_DrawInt32(
    tft_ili9341_t *tft,
    int32_t x,
    int32_t y,
    int32_t value,
    tft_ili9341_font_t font,
    uint16_t foreground,
    uint16_t background,
    bool transparent_background)
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
    position = tft_append_uint32(text, position, magnitude, 1U);
    text[position] = '\0';

    return TFT_ILI9341_DrawString(
        tft, x, y, text, font, foreground, background,
        transparent_background, false);
}

tft_ili9341_status_t TFT_ILI9341_DrawFloat(
    tft_ili9341_t *tft,
    int32_t x,
    int32_t y,
    float value,
    uint8_t decimal_places,
    tft_ili9341_font_t font,
    uint16_t foreground,
    uint16_t background,
    bool transparent_background)
{
    char text[32];
    size_t position = 0U;
    float magnitude;
    uint32_t whole;
    uint32_t fractional;
    uint32_t scale = 1U;
    uint8_t index;

    if (decimal_places > 6U) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    if (value != value) {
        return TFT_ILI9341_DrawString(
            tft, x, y, "nan", font, foreground, background,
            transparent_background, false);
    }

    if (value < 0.0F) {
        text[position++] = '-';
        magnitude = -value;
    } else {
        magnitude = value;
    }
    if (magnitude > 4294967040.0F) {
        return TFT_ILI9341_ERROR_ARGUMENT;
    }
    for (index = 0U; index < decimal_places; ++index) {
        scale *= 10U;
    }

    whole = (uint32_t)magnitude;
    fractional = (uint32_t)(((magnitude - (float)whole) * (float)scale) +
                            0.5F);
    if (fractional >= scale) {
        if (whole == UINT32_MAX) {
            return TFT_ILI9341_ERROR_ARGUMENT;
        }
        ++whole;
        fractional = 0U;
    }

    position = tft_append_uint32(text, position, whole, 1U);
    if (decimal_places != 0U) {
        text[position++] = '.';
        position = tft_append_uint32(text, position, fractional,
                                     decimal_places);
    }
    text[position] = '\0';

    return TFT_ILI9341_DrawString(
        tft, x, y, text, font, foreground, background,
        transparent_background, false);
}

signal_module_status_t SignalTFTILI9341_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
