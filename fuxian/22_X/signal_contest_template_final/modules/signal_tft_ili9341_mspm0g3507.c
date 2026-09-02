/**
 * @file signal_tft_ili9341_mspm0g3507.c
 * @brief MSPM0G3507 contest entry for the ILI9341 driver.
 * @note Original path: MSPM0_Signal_Contest/01_bsp/tft_ili9341/
 */

#include "signal_tft_ili9341_mspm0g3507.h"

#include <stdbool.h>
#include <stddef.h>

#include "ti_msp_dl_config.h"

static int SignalTFTMSPM0_Write(
    void *context, const uint8_t *data, size_t length)
{
    size_t index;
    (void) context;
    if ((data == NULL) && (length != 0U)) return -1;
    for (index = 0U; index < length; ++index) {
        DL_SPI_transmitDataBlocking8(SPI_TFT_INST, data[index]);
        while (!DL_SPI_isRXFIFOEmpty(SPI_TFT_INST)) {
            (void) DL_SPI_receiveData8(SPI_TFT_INST);
        }
    }
    return 0;
}

static void SignalTFTMSPM0_SetDC(void *context, bool high)
{
    (void) context;
    if (high) {
        DL_GPIO_setPins(GPIO_TFT_CTRL_PORT, GPIO_TFT_CTRL_TFT_DC_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_TFT_CTRL_PORT, GPIO_TFT_CTRL_TFT_DC_PIN);
    }
}

static void SignalTFTMSPM0_SetBacklight(void *context, bool high)
{
    (void) context;
    if (high) {
        DL_GPIO_setPins(GPIO_TFT_CTRL_PORT, GPIO_TFT_CTRL_TFT_BLK_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_TFT_CTRL_PORT, GPIO_TFT_CTRL_TFT_BLK_PIN);
    }
}

static void SignalTFTMSPM0_DelayMs(void *context, uint32_t milliseconds)
{
    (void) context;
    while (milliseconds != 0U) {
        delay_cycles(CPUCLK_FREQ / 1000U);
        --milliseconds;
    }
}

tft_ili9341_status_t SignalTFTILI9341_MSPM0_Init(
    tft_ili9341_t *tft, tft_ili9341_rotation_t rotation)
{
    const tft_ili9341_config_t config = {
        .context = NULL,
        .write = SignalTFTMSPM0_Write,
        .set_cs = NULL, /* SPI_TFT hardware CS owns the CS pin. */
        .set_dc = SignalTFTMSPM0_SetDC,
        .set_reset = NULL,
        .set_backlight = SignalTFTMSPM0_SetBacklight,
        .delay_ms = SignalTFTMSPM0_DelayMs,
        .lock = NULL,
        .unlock = NULL,
    };
    SignalTFTMSPM0_DelayMs(NULL, 350U);
    return TFT_ILI9341_Init(tft, &config, rotation);
}
