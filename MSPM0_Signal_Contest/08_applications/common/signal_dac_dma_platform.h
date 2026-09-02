#ifndef SIGNAL_DAC_DMA_PLATFORM_H
#define SIGNAL_DAC_DMA_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "signal_status.h"

signal_result_t SignalDACPlatform_Init(uint32_t update_rate_hz,
    uint32_t timer_clock_hz);
signal_result_t SignalDACPlatform_Start(void *context,
    const uint16_t *samples, size_t count, bool repeat);
signal_result_t SignalDACPlatform_Stop(void *context);
bool SignalDACPlatform_IsOneShotFinished(void);
uint32_t SignalDACPlatform_GetConfiguredRate(void);

#endif
