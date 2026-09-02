#include "signal_spectrum_analyzer.h"

#include "signal_status_adapter.h"

signal_result_t SignalSpectrumAnalyzer_Analyze(float *voltage_workspace,
    size_t count, float sample_rate_hz, float expected_min_hz,
    float expected_max_hz, signal_complex_f32_t *fft_workspace,
    size_t fft_capacity, float *magnitude_workspace,
    size_t magnitude_capacity, uint32_t requested_peak_count,
    signal_spectrum_integration_result_t *result)
{
    return SignalStatus_FromAlgorithm(SignalIntegration_Spectrum(
        voltage_workspace, count, sample_rate_hz, expected_min_hz,
        expected_max_hz, fft_workspace, fft_capacity, magnitude_workspace,
        magnitude_capacity, requested_peak_count, result));
}

signal_module_status_t SignalSpectrumAnalyzer_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
