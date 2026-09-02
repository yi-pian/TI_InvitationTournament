#include "signal_tft_st7789_mspm0g3507.h"

#include "ti_msp_dl_config.h"

static int write_spi(void *context, const uint8_t *data, size_t length)
{
    size_t i; (void)context;
    if (data == NULL && length != 0U) return -1;
    for (i=0U;i<length;++i) {
        DL_SPI_transmitDataBlocking8(SPI_TFT_INST, data[i]);
        (void)DL_SPI_receiveDataBlocking8(SPI_TFT_INST);
    }
    return 0;
}

static void set_dc(void *context, bool high)
{
    (void)context;
    if (high) DL_GPIO_setPins(GPIO_TFT_CTRL_PORT, GPIO_TFT_CTRL_TFT_DC_PIN);
    else DL_GPIO_clearPins(GPIO_TFT_CTRL_PORT, GPIO_TFT_CTRL_TFT_DC_PIN);
}

static void set_bl(void *context, bool high)
{
    (void)context;
    if (high) DL_GPIO_setPins(GPIO_TFT_CTRL_PORT, GPIO_TFT_CTRL_TFT_BLK_PIN);
    else DL_GPIO_clearPins(GPIO_TFT_CTRL_PORT, GPIO_TFT_CTRL_TFT_BLK_PIN);
}

#if defined(GPIO_TFT_CTRL_TFT_RST_PIN)
static void set_reset(void *context, bool high)
{
    (void)context;
    if (high) DL_GPIO_setPins(GPIO_TFT_CTRL_PORT, GPIO_TFT_CTRL_TFT_RST_PIN);
    else DL_GPIO_clearPins(GPIO_TFT_CTRL_PORT, GPIO_TFT_CTRL_TFT_RST_PIN);
}
#endif

static void delay_ms(void *context, uint32_t milliseconds)
{
    (void)context;
    while (milliseconds-- != 0U) delay_cycles(CPUCLK_FREQ / 1000U);
}

tft_st7789_status_t SignalTFTST7789_MSPM0_Init(
    tft_st7789_t *tft, tft_st7789_rotation_t rotation,
    uint16_t x_offset, uint16_t y_offset)
{
    const tft_st7789_config_t config = {
        .context = NULL, .write = write_spi, .set_cs = NULL,
        .set_dc = set_dc,
#if defined(GPIO_TFT_CTRL_TFT_RST_PIN)
        .set_reset = set_reset,
#else
        .set_reset = NULL,
#endif
        .set_backlight = set_bl, .delay_ms = delay_ms,
        .lock = NULL, .unlock = NULL,
        .x_offset = x_offset, .y_offset = y_offset
    };
    return TFT_ST7789_Init(tft, &config, rotation);
}
