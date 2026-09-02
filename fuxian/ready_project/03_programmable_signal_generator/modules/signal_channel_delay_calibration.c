#include "signal_channel_delay_calibration.h"

#include <math.h>
#include <stddef.h>

static float SignalChannelDelay_WrapDegrees(float phase_deg)
{
    float wrapped = fmodf(phase_deg + 180.0f, 360.0f);
    if (wrapped < 0.0f)
    {
        wrapped += 360.0f;
    }
    return wrapped - 180.0f;
}

signal_algorithm_status_t SignalChannelDelayCalibration_Compute(
    float measured_phase_b_minus_a_deg,
    float expected_phase_b_minus_a_deg,
    float frequency_hz,
    signal_channel_delay_calibration_t *calibration)
{
    float phase_error_deg;

    if (calibration == NULL)
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (!isfinite(measured_phase_b_minus_a_deg) ||
        !isfinite(expected_phase_b_minus_a_deg) ||
        !isfinite(frequency_hz) || (frequency_hz <= 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    phase_error_deg = SignalChannelDelay_WrapDegrees(
        measured_phase_b_minus_a_deg - expected_phase_b_minus_a_deg);
    calibration->delay_b_relative_to_a_s =
        -phase_error_deg / (360.0f * frequency_hz);
    return isfinite(calibration->delay_b_relative_to_a_s)
               ? SIGNAL_ALGORITHM_OK
               : SIGNAL_ALGORITHM_NUMERIC_ERROR;
}

signal_algorithm_status_t SignalChannelDelayCalibration_Apply(
    float measured_phase_b_minus_a_deg,
    float frequency_hz,
    const signal_channel_delay_calibration_t *calibration,
    float *corrected_phase_b_minus_a_deg)
{
    float corrected;

    if ((calibration == NULL) || (corrected_phase_b_minus_a_deg == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (!isfinite(measured_phase_b_minus_a_deg) ||
        !isfinite(frequency_hz) || (frequency_hz <= 0.0f) ||
        !isfinite(calibration->delay_b_relative_to_a_s))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    corrected = measured_phase_b_minus_a_deg +
        (360.0f * frequency_hz * calibration->delay_b_relative_to_a_s);
    *corrected_phase_b_minus_a_deg = SignalChannelDelay_WrapDegrees(corrected);
    return SIGNAL_ALGORITHM_OK;
}
