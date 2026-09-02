#ifndef SIGNAL_FREQUENCY_METER_H
#define SIGNAL_FREQUENCY_METER_H

#include <stddef.h>
#include <stdint.h>

#include "signal_status.h"
#include "signal_zero_cross.h"

typedef struct {
    float frequency_hz;
    float mean_period_samples;
    uint32_t crossing_count;
} signal_frequency_meter_waveform_result_t;

signal_result_t SignalFrequencyMeter_FromWaveform(const float *samples,
    size_t count, float sample_rate_hz, float threshold_v,
    float hysteresis_v, signal_zero_cross_event_t *events,
    size_t event_capacity, float *crossing_positions,
    size_t position_capacity, signal_frequency_meter_waveform_result_t *result);

signal_result_t SignalFrequencyMeter_FromCapture(const uint32_t *timestamps,
    size_t count, uint32_t timer_hz, uint32_t counter_modulus,
    float *frequency_hz, float *period_ticks);

signal_module_status_t SignalFrequencyMeter_GetModuleStatus(void);

#endif
