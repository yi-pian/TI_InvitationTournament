#ifndef SIGNAL_DUAL_ADC_PLATFORM_H
#define SIGNAL_DUAL_ADC_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include "signal_status.h"

signal_result_t SignalDualADCPlatform_Init(uint32_t sample_rate_hz,
    uint32_t timer_clock_hz);
signal_result_t SignalDualADCPlatform_Start(uint16_t *channel_a,
    uint16_t *channel_b, uint16_t sample_count);
bool SignalDualADCPlatform_IsFinished(void);
void SignalDualADCPlatform_Stop(void);
uint32_t SignalDualADCPlatform_GetConfiguredRate(void);

#endif
