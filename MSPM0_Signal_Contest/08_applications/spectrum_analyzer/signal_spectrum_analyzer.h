#ifndef SIGNAL_SPECTRUM_ANALYZER_H
#define SIGNAL_SPECTRUM_ANALYZER_H

#include <stddef.h>
#include <stdint.h>

#include "signal_complex.h"
#include "signal_integration.h"
#include "signal_status.h"

signal_result_t SignalSpectrumAnalyzer_Analyze(float *voltage_workspace,
    size_t count, float sample_rate_hz, float expected_min_hz,
    float expected_max_hz, signal_complex_f32_t *fft_workspace,
    size_t fft_capacity, float *magnitude_workspace,
    size_t magnitude_capacity, uint32_t requested_peak_count,
    signal_spectrum_integration_result_t *result);

signal_module_status_t SignalSpectrumAnalyzer_GetModuleStatus(void);

#endif
