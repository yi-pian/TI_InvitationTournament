#include "signal_harmonic_thd_analyzer.h"

#include <limits.h>

#include "signal_status_adapter.h"

signal_result_t SignalHarmonicTHDAnalyzer_Analyze(const float *magnitude,
    size_t bin_count, float sample_rate_hz, uint32_t fft_size,
    float fundamental_frequency_hz, uint32_t bin_radius,
    uint32_t last_harmonic_order, signal_harmonic_thd_result_t *result)
{
    signal_harmonic_config_t config;
    signal_algorithm_status_t status;

    if ((result == NULL) || (bin_count > UINT32_MAX)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    config.fundamental_frequency_hz = fundamental_frequency_hz;
    config.first_order = 1U;
    config.last_order = last_harmonic_order;
    config.radius_bins = bin_radius;
    status = SignalHarmonic_Process(magnitude, (uint32_t) bin_count,
        sample_rate_hz, fft_size, &config, &result->harmonics);
    if (status != SIGNAL_ALGORITHM_OK) return SignalStatus_FromAlgorithm(status);
    status = SignalTHD_Process(&result->harmonics, &result->thd);
    return SignalStatus_FromAlgorithm(status);
}

signal_module_status_t SignalHarmonicTHDAnalyzer_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
