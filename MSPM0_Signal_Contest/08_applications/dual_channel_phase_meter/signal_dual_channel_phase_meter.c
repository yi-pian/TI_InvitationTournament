#include "signal_dual_channel_phase_meter.h"

#include <limits.h>

#include "signal_correlation.h"
#include "signal_phase.h"
#include "signal_status_adapter.h"

signal_result_t SignalDualChannelPhaseMeter_Measure(const float *channel_a,
    const float *channel_b, size_t count, uint32_t maximum_absolute_lag,
    float sample_rate_hz, float signal_frequency_hz,
    float *correlation_workspace, size_t correlation_capacity,
    signal_dual_channel_phase_result_t *result)
{
    signal_correlation_result_t correlation;
    signal_phase_result_t phase;
    signal_algorithm_status_t status;

    if ((result == NULL) || (count > UINT32_MAX) ||
        (correlation_capacity > UINT32_MAX) || !(sample_rate_hz > 0.0f) ||
        !(signal_frequency_hz > 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    status = SignalCorrelation_Process(channel_a, channel_b, (uint32_t) count,
        maximum_absolute_lag, correlation_workspace,
        (uint32_t) correlation_capacity, &correlation);
    if (status != SIGNAL_ALGORITHM_OK) return SignalStatus_FromAlgorithm(status);
    status = SignalPhase_FromCorrelationLag(
        (float) correlation.best_lag_samples,
        sample_rate_hz / signal_frequency_hz, &phase);
    if (status != SIGNAL_ALGORITHM_OK) return SignalStatus_FromAlgorithm(status);

    result->delay_samples = (float) correlation.best_lag_samples;
    result->phase_degrees = phase.phase_difference_deg;
    result->correlation = correlation.best_coefficient;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalDualChannelPhaseMeter_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
