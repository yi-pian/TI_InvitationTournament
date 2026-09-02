#include "signal_frequency_response_correction.h"

#include <math.h>
#include <stddef.h>

static float signal_frc_wrap_degrees(float angle)
{
    while (angle > 180.0f)
    {
        angle -= 360.0f;
    }
    while (angle <= -180.0f)
    {
        angle += 360.0f;
    }
    return angle;
}

signal_algorithm_status_t SignalFrequencyResponseCorrection_Process(
    const signal_frequency_response_correction_point_t *table,
    uint32_t table_count,
    float frequency_hz,
    float measured_gain_linear,
    float measured_phase_deg,
    signal_frc_interpolation_t interpolation,
    signal_frc_range_policy_t range_policy,
    signal_frequency_response_correction_result_t *result)
{
    signal_frequency_response_correction_result_t temporary;
    uint32_t index;
    uint32_t lower;
    uint32_t upper;
    float fraction;
    float phase_delta;

    if ((table == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (table_count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(frequency_hz) || !isfinite(measured_gain_linear) ||
        !isfinite(measured_phase_deg) || (frequency_hz <= 0.0f) ||
        (measured_gain_linear < 0.0f) ||
        ((interpolation != SIGNAL_FRC_INTERPOLATE_LINEAR_HZ) &&
         (interpolation != SIGNAL_FRC_INTERPOLATE_LOG_HZ)) ||
        ((range_policy != SIGNAL_FRC_RANGE_REJECT) &&
         (range_policy != SIGNAL_FRC_RANGE_CLAMP)))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (index = 0U; index < table_count; ++index)
    {
        if (!isfinite(table[index].frequency_hz) ||
            !isfinite(table[index].gain_correction_linear) ||
            !isfinite(table[index].phase_correction_deg) ||
            (table[index].frequency_hz <= 0.0f) ||
            (table[index].gain_correction_linear <= 0.0f) ||
            ((index > 0U) &&
             (table[index].frequency_hz <= table[index - 1U].frequency_hz)))
        {
            return SIGNAL_ALGORITHM_OUT_OF_RANGE;
        }
    }

    if (frequency_hz <= table[0].frequency_hz)
    {
        if ((frequency_hz < table[0].frequency_hz) &&
            (range_policy == SIGNAL_FRC_RANGE_REJECT))
        {
            return SIGNAL_ALGORITHM_OUT_OF_RANGE;
        }
        lower = 0U;
        upper = 0U;
        fraction = 0.0f;
    }
    else if (frequency_hz >= table[table_count - 1U].frequency_hz)
    {
        if ((frequency_hz > table[table_count - 1U].frequency_hz) &&
            (range_policy == SIGNAL_FRC_RANGE_REJECT))
        {
            return SIGNAL_ALGORITHM_OUT_OF_RANGE;
        }
        lower = table_count - 1U;
        upper = lower;
        fraction = 0.0f;
    }
    else
    {
        upper = 1U;
        while (table[upper].frequency_hz < frequency_hz)
        {
            ++upper;
        }
        lower = upper - 1U;
        if (interpolation == SIGNAL_FRC_INTERPOLATE_LOG_HZ)
        {
            fraction = (logf(frequency_hz) - logf(table[lower].frequency_hz)) /
                (logf(table[upper].frequency_hz) -
                 logf(table[lower].frequency_hz));
        }
        else
        {
            fraction = (frequency_hz - table[lower].frequency_hz) /
                (table[upper].frequency_hz - table[lower].frequency_hz);
        }
    }

    temporary.applied_gain_correction_linear =
        table[lower].gain_correction_linear + fraction *
        (table[upper].gain_correction_linear -
         table[lower].gain_correction_linear);
    phase_delta = signal_frc_wrap_degrees(
        table[upper].phase_correction_deg -
        table[lower].phase_correction_deg);
    temporary.applied_phase_correction_deg = signal_frc_wrap_degrees(
        table[lower].phase_correction_deg + (fraction * phase_delta));
    temporary.corrected_gain_linear = measured_gain_linear *
                                      temporary.applied_gain_correction_linear;
    temporary.corrected_phase_deg = signal_frc_wrap_degrees(
        measured_phase_deg + temporary.applied_phase_correction_deg);
    temporary.interpolation_fraction = fraction;
    temporary.lower_index = lower;
    temporary.upper_index = upper;
    if (!isfinite(temporary.corrected_gain_linear) ||
        !isfinite(temporary.corrected_phase_deg))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    *result = temporary;
    return SIGNAL_ALGORITHM_OK;
}
