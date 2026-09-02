#ifndef SIGNAL_SIGNAL_ANALYZER_H
#define SIGNAL_SIGNAL_ANALYZER_H

#include <stddef.h>

#include "signal_frequency_meter.h"
#include "signal_oscilloscope.h"
#include "signal_status.h"

typedef struct {
    signal_oscilloscope_measurements_t time_domain;
    signal_frequency_meter_waveform_result_t frequency;
} signal_analyzer_result_t;

signal_result_t SignalAnalyzer_Analyze(const float *voltage_v, size_t count,
    float sample_rate_hz, float crossing_threshold_v,
    float crossing_hysteresis_v, signal_zero_cross_event_t *events,
    size_t event_capacity, float *crossing_positions,
    size_t position_capacity, signal_analyzer_result_t *result);

signal_module_status_t SignalAnalyzer_GetModuleStatus(void);

#endif
