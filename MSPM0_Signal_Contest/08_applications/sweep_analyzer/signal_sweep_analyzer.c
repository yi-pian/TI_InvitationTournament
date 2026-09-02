#include "signal_sweep_analyzer.h"

#include <math.h>
#include <stddef.h>

signal_result_t SignalSweepAnalyzer_Point(float reference_amplitude,
    float response_amplitude, float phase_degrees,
    signal_sweep_point_result_t *result)
{
    return SignalSweepAnalyzer_PointAtFrequency(0.0f, reference_amplitude,
        response_amplitude, phase_degrees, result);
}

signal_result_t SignalSweepAnalyzer_PointAtFrequency(float frequency_hz,
    float reference_amplitude, float response_amplitude,
    float phase_degrees, signal_sweep_point_result_t *result)
{
    if ((result == NULL) || !(reference_amplitude > 0.0f) ||
        !(response_amplitude > 0.0f) || (frequency_hz < 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    result->frequency_hz = frequency_hz;
    result->reference_amplitude = reference_amplitude;
    result->response_amplitude = response_amplitude;
    result->gain_linear = response_amplitude / reference_amplitude;
    result->gain_db = 20.0f * log10f(result->gain_linear);
    result->phase_degrees = phase_degrees;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalSweepAnalyzer_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
