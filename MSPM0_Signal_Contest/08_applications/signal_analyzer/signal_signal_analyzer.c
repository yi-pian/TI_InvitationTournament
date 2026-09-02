#include "signal_signal_analyzer.h"

signal_result_t SignalAnalyzer_Analyze(const float *voltage_v, size_t count,
    float sample_rate_hz, float crossing_threshold_v,
    float crossing_hysteresis_v, signal_zero_cross_event_t *events,
    size_t event_capacity, float *crossing_positions,
    size_t position_capacity, signal_analyzer_result_t *result)
{
    signal_result_t status;

    if (result == NULL) return SIGNAL_RESULT_INVALID_ARGUMENT;
    status = SignalOscilloscope_Analyze(
        voltage_v, count, &result->time_domain);
    if (status != SIGNAL_RESULT_OK) return status;
    return SignalFrequencyMeter_FromWaveform(voltage_v, count,
        sample_rate_hz, crossing_threshold_v, crossing_hysteresis_v,
        events, event_capacity, crossing_positions, position_capacity,
        &result->frequency);
}

signal_module_status_t SignalAnalyzer_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
