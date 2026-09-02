#ifndef SIGNAL_MSPM0G3507_TFT_PLATFORM_H
#define SIGNAL_MSPM0G3507_TFT_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include <ti/driverlib/driverlib.h>

#include "signal_tft_ili9341.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    SPI_Regs *spi;
    GPIO_Regs *cs_port;
    uint32_t cs_pin;
    GPIO_Regs *dc_port;
    uint32_t dc_pin;
    GPIO_Regs *reset_port;
    uint32_t reset_pin;
    GPIO_Regs *backlight_port;
    uint32_t backlight_pin;
    uint32_t cpu_clock_hz;
} signal_mspm0g3507_tft_context_t;

/* Bind callbacks into config. Zero port/pin means that optional line is absent. */
signal_result_t SignalMSPM0G3507_TFT_Bind(
    tft_ili9341_config_t *config,
    signal_mspm0g3507_tft_context_t *context,
    bool hardware_chip_select);
int SignalMSPM0G3507_TFT_Write(void *context, const uint8_t *data,
    size_t length);
void SignalMSPM0G3507_TFT_SetCS(void *context, bool high);
void SignalMSPM0G3507_TFT_SetDC(void *context, bool high);
void SignalMSPM0G3507_TFT_SetReset(void *context, bool high);
void SignalMSPM0G3507_TFT_SetBacklight(void *context, bool high);
void SignalMSPM0G3507_TFT_DelayMs(void *context, uint32_t milliseconds);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_MSPM0G3507_TFT_PLATFORM_H */
