#ifndef SIGNAL_SWEEP_ANALYZER_H
#define SIGNAL_SWEEP_ANALYZER_H

#include <stddef.h>
#include "signal_status.h"

typedef struct {
    float frequency_hz;
    float reference_amplitude;
    float response_amplitude;
    float gain_linear;
    float gain_db;
    float phase_degrees;
} signal_sweep_point_result_t;

signal_result_t SignalSweepAnalyzer_Point(float reference_amplitude,
    float response_amplitude, float phase_degrees,
    signal_sweep_point_result_t *result);
signal_result_t SignalSweepAnalyzer_PointAtFrequency(float frequency_hz,
    float reference_amplitude, float response_amplitude,
    float phase_degrees, signal_sweep_point_result_t *result);
signal_module_status_t SignalSweepAnalyzer_GetModuleStatus(void);

#endif
