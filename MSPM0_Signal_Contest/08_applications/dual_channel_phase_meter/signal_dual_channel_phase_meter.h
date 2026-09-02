#ifndef SIGNAL_DUAL_CHANNEL_PHASE_METER_H
#define SIGNAL_DUAL_CHANNEL_PHASE_METER_H

#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

typedef struct {
    float delay_samples;
    float phase_degrees;
    float correlation;
} signal_dual_channel_phase_result_t;

signal_result_t SignalDualChannelPhaseMeter_Measure(const float *channel_a,
    const float *channel_b, size_t count, uint32_t maximum_absolute_lag,
    float sample_rate_hz, float signal_frequency_hz,
    float *correlation_workspace, size_t correlation_capacity,
    signal_dual_channel_phase_result_t *result);
signal_module_status_t SignalDualChannelPhaseMeter_GetModuleStatus(void);

#endif
