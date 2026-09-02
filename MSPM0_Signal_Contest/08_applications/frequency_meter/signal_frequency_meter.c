#include "signal_frequency_meter.h"

#include <limits.h>

#include "signal_multi_cycle_average.h"
#include "signal_status_adapter.h"
#include "signal_timer_capture.h"
#include "signal_zero_cross_interpolation.h"

signal_result_t SignalFrequencyMeter_FromWaveform(const float *samples,
    size_t count, float sample_rate_hz, float threshold_v,
    float hysteresis_v, signal_zero_cross_event_t *events,
    size_t event_capacity, float *crossing_positions,
    size_t position_capacity, signal_frequency_meter_waveform_result_t *result)
{
    signal_zero_cross_config_t config;
    signal_zero_cross_result_t crossing_result;
    signal_zero_cross_interpolation_result_t interpolation_result;
    signal_multi_cycle_average_result_t average_result;
    signal_algorithm_status_t status;

    if ((result == NULL) || (count > UINT32_MAX) ||
        (event_capacity > UINT32_MAX) || (position_capacity > UINT32_MAX)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    config.threshold_v = threshold_v;
    config.hysteresis_v = hysteresis_v;
    config.direction = SIGNAL_ZERO_CROSS_RISING;
    status = SignalZeroCross_Process(samples, (uint32_t) count, &config,
        events, (uint32_t) event_capacity, &crossing_result);
    if (status != SIGNAL_ALGORITHM_OK) return SignalStatus_FromAlgorithm(status);
    status = SignalZeroCrossInterpolation_Process(samples, (uint32_t) count,
        threshold_v, events, crossing_result.event_count, crossing_positions,
        (uint32_t) position_capacity, &interpolation_result);
    if (status != SIGNAL_ALGORITHM_OK) return SignalStatus_FromAlgorithm(status);
    status = SignalMultiCycleAverage_Process(crossing_positions,
        interpolation_result.position_count, sample_rate_hz, &average_result);
    if (status != SIGNAL_ALGORITHM_OK) return SignalStatus_FromAlgorithm(status);

    result->frequency_hz = average_result.frequency_hz;
    result->mean_period_samples = average_result.average_period_samples;
    result->crossing_count = interpolation_result.position_count;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalFrequencyMeter_FromCapture(const uint32_t *timestamps,
    size_t count, uint32_t timer_hz, uint32_t counter_modulus,
    float *frequency_hz, float *period_ticks)
{
    const signal_timer_capture_config_t config = {
        timer_hz, counter_modulus
    };
    return SignalTimerCapture_MeanPeriod(timestamps, count, &config,
        period_ticks, frequency_hz);
}

signal_module_status_t SignalFrequencyMeter_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
