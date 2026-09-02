#include "signal_mspm0g3507_tft_platform.h"

#include <stddef.h>

static void SignalMSPM0G3507_TFT_WritePin(GPIO_Regs *port, uint32_t pin,
    bool high)
{
    if ((port == NULL) || (pin == 0U)) return;
    if (high) DL_GPIO_setPins(port, pin);
    else DL_GPIO_clearPins(port, pin);
}

signal_result_t SignalMSPM0G3507_TFT_Bind(
    tft_ili9341_config_t *config,
    signal_mspm0g3507_tft_context_t *context,
    bool hardware_chip_select)
{
    if ((config == NULL) || (context == NULL) || (context->spi == NULL) ||
        (context->dc_port == NULL) || (context->dc_pin == 0U) ||
        (context->cpu_clock_hz < 1000U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    config->context = context;
    config->write = SignalMSPM0G3507_TFT_Write;
    config->set_cs = hardware_chip_select ? NULL :
        SignalMSPM0G3507_TFT_SetCS;
    config->set_dc = SignalMSPM0G3507_TFT_SetDC;
    config->set_reset = ((context->reset_port == NULL) ||
        (context->reset_pin == 0U)) ? NULL : SignalMSPM0G3507_TFT_SetReset;
    config->set_backlight = ((context->backlight_port == NULL) ||
        (context->backlight_pin == 0U)) ? NULL :
        SignalMSPM0G3507_TFT_SetBacklight;
    config->delay_ms = SignalMSPM0G3507_TFT_DelayMs;
    config->lock = NULL;
    config->unlock = NULL;
    return SIGNAL_RESULT_OK;
}

int SignalMSPM0G3507_TFT_Write(void *context, const uint8_t *data,
    size_t length)
{
    signal_mspm0g3507_tft_context_t *tft =
        (signal_mspm0g3507_tft_context_t *) context;
    size_t index;
    if ((tft == NULL) || (tft->spi == NULL) ||
        ((data == NULL) && (length != 0U))) return -1;
    for (index = 0U; index < length; ++index) {
        DL_SPI_transmitDataBlocking8(tft->spi, data[index]);
        while (!DL_SPI_isRXFIFOEmpty(tft->spi)) {
            (void) DL_SPI_receiveData8(tft->spi);
        }
    }
    return 0;
}

void SignalMSPM0G3507_TFT_SetCS(void *context, bool high)
{
    signal_mspm0g3507_tft_context_t *tft =
        (signal_mspm0g3507_tft_context_t *) context;
    if (tft != NULL) {
        SignalMSPM0G3507_TFT_WritePin(tft->cs_port, tft->cs_pin, high);
    }
}

void SignalMSPM0G3507_TFT_SetDC(void *context, bool high)
{
    signal_mspm0g3507_tft_context_t *tft =
        (signal_mspm0g3507_tft_context_t *) context;
    if (tft != NULL) {
        SignalMSPM0G3507_TFT_WritePin(tft->dc_port, tft->dc_pin, high);
    }
}

void SignalMSPM0G3507_TFT_SetReset(void *context, bool high)
{
    signal_mspm0g3507_tft_context_t *tft =
        (signal_mspm0g3507_tft_context_t *) context;
    if (tft != NULL) {
        SignalMSPM0G3507_TFT_WritePin(tft->reset_port, tft->reset_pin, high);
    }
}

void SignalMSPM0G3507_TFT_SetBacklight(void *context, bool high)
{
    signal_mspm0g3507_tft_context_t *tft =
        (signal_mspm0g3507_tft_context_t *) context;
    if (tft != NULL) {
        SignalMSPM0G3507_TFT_WritePin(tft->backlight_port,
            tft->backlight_pin, high);
    }
}

void SignalMSPM0G3507_TFT_DelayMs(void *context, uint32_t milliseconds)
{
    signal_mspm0g3507_tft_context_t *tft =
        (signal_mspm0g3507_tft_context_t *) context;
    if ((tft == NULL) || (tft->cpu_clock_hz < 1000U)) return;
    while (milliseconds != 0U) {
        delay_cycles(tft->cpu_clock_hz / 1000U);
        --milliseconds;
    }
}
