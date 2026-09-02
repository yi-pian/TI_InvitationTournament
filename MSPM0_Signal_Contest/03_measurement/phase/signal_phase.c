#include "signal_phase.h"
#include "signal_math_backend.h"

#include <math.h>
#include <stddef.h>

#define SIGNAL_PHASE_PI_F 3.14159265358979323846f

static float SignalPhase_WrapDegrees(float phase_deg)
{
    float wrapped = fmodf(phase_deg + 180.0f, 360.0f);
    if (wrapped < 0.0f)
    {
        wrapped += 360.0f;
    }
    return wrapped - 180.0f;
}

static signal_algorithm_status_t SignalPhase_SetResult(
    float phase_deg,
    signal_phase_result_t *result)
{
    if ((result == NULL) || !isfinite(phase_deg))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    result->phase_difference_deg = SignalPhase_WrapDegrees(phase_deg);
    result->phase_difference_rad = result->phase_difference_deg *
                                   SIGNAL_PHASE_PI_F / 180.0f;
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalPhase_FromZeroCross(
    float crossing_a_samples,
    float crossing_b_samples,
    float period_samples,
    signal_phase_result_t *result)
{
    if (!isfinite(crossing_a_samples) || !isfinite(crossing_b_samples) ||
        !isfinite(period_samples) || (period_samples <= 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    return SignalPhase_SetResult(
        -360.0f * (crossing_b_samples - crossing_a_samples) / period_samples,
        result);
}

signal_algorithm_status_t SignalPhase_FromFFTBin(
    const signal_complex_f32_t *spectrum_a,
    const signal_complex_f32_t *spectrum_b,
    uint32_t spectrum_count,
    uint32_t bin_index,
    signal_phase_result_t *result)
{
    float magnitude_square_a;
    float magnitude_square_b;
    float phase_a;
    float phase_b;

    if ((spectrum_a == NULL) || (spectrum_b == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((spectrum_count == 0U) || (bin_index >= spectrum_count))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    if (!isfinite(spectrum_a[bin_index].real) ||
        !isfinite(spectrum_a[bin_index].imag) ||
        !isfinite(spectrum_b[bin_index].real) ||
        !isfinite(spectrum_b[bin_index].imag))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    magnitude_square_a = spectrum_a[bin_index].real * spectrum_a[bin_index].real +
                         spectrum_a[bin_index].imag * spectrum_a[bin_index].imag;
    magnitude_square_b = spectrum_b[bin_index].real * spectrum_b[bin_index].real +
                         spectrum_b[bin_index].imag * spectrum_b[bin_index].imag;
    if ((magnitude_square_a <= 0.0f) || (magnitude_square_b <= 0.0f))
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }
    phase_a = SignalMathBackend_Atan2F(spectrum_a[bin_index].imag,
                                      spectrum_a[bin_index].real);
    phase_b = SignalMathBackend_Atan2F(spectrum_b[bin_index].imag,
                                      spectrum_b[bin_index].real);
    return SignalPhase_SetResult(
        (phase_b - phase_a) * 180.0f / SIGNAL_PHASE_PI_F,
        result);
}

signal_algorithm_status_t SignalPhase_FromCorrelationLag(
    float lag_b_relative_to_a_samples,
    float period_samples,
    signal_phase_result_t *result)
{
    if (!isfinite(lag_b_relative_to_a_samples) ||
        !isfinite(period_samples) || (period_samples <= 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    return SignalPhase_SetResult(
        -360.0f * lag_b_relative_to_a_samples / period_samples,
        result);
}
