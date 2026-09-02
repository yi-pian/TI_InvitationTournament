#ifndef SIGNAL_HARMONIC_THD_ANALYZER_H
#define SIGNAL_HARMONIC_THD_ANALYZER_H

#include <stddef.h>
#include <stdint.h>

#include "signal_harmonic.h"
#include "signal_status.h"
#include "signal_thd.h"

typedef struct {
    signal_harmonic_result_t harmonics;
    signal_thd_result_t thd;
} signal_harmonic_thd_result_t;

signal_result_t SignalHarmonicTHDAnalyzer_Analyze(const float *magnitude,
    size_t bin_count, float sample_rate_hz, uint32_t fft_size,
    float fundamental_frequency_hz, uint32_t bin_radius,
    uint32_t last_harmonic_order, signal_harmonic_thd_result_t *result);

signal_module_status_t SignalHarmonicTHDAnalyzer_GetModuleStatus(void);

#endif
