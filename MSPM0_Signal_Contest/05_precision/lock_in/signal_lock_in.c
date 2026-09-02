#include "signal_lock_in.h"

#include <math.h>
#include <stddef.h>

#define SIGNAL_LOCK_IN_PI_F 3.14159265358979323846f

signal_algorithm_status_t SignalLockIn_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_lock_in_config_t *config,
    signal_lock_in_result_t *result)
{
    uint32_t index;
    float mean = 0.0f;
    float step;
    float step_cos;
    float step_sin;
    float reference_cos;
    float reference_sin;
    float in_phase_sum = 0.0f;
    float quadrature_sum = 0.0f;

    if ((voltage_v == NULL) || (config == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count < 2U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(config->reference_frequency_hz) ||
        !isfinite(config->sample_rate_hz) ||
        !isfinite(config->reference_phase_rad) ||
        (config->reference_frequency_hz <= 0.0f) ||
        (config->sample_rate_hz <= 0.0f) ||
        (config->reference_frequency_hz >= (0.5f * config->sample_rate_hz)) ||
        (config->remove_dc > 1U))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(voltage_v[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        mean += voltage_v[index];
    }
    mean /= (float)count;
    step = 2.0f * SIGNAL_LOCK_IN_PI_F * config->reference_frequency_hz /
           config->sample_rate_hz;
    step_cos = cosf(step);
    step_sin = sinf(step);
    reference_cos = cosf(config->reference_phase_rad);
    reference_sin = sinf(config->reference_phase_rad);

    for (index = 0U; index < count; ++index)
    {
        float sample = voltage_v[index] -
            ((config->remove_dc != 0U) ? mean : 0.0f);
        float next_cos;
        in_phase_sum += sample * reference_cos;
        quadrature_sum -= sample * reference_sin;
        next_cos = reference_cos * step_cos - reference_sin * step_sin;
        reference_sin = reference_cos * step_sin + reference_sin * step_cos;
        reference_cos = next_cos;
    }
    result->mean_voltage_v = mean;
    result->in_phase_v = 2.0f * in_phase_sum / (float)count;
    result->quadrature_v = 2.0f * quadrature_sum / (float)count;
    result->amplitude_peak_v = hypotf(result->in_phase_v,
                                      result->quadrature_v);
    result->phase_rad = atan2f(result->quadrature_v, result->in_phase_v);
    result->phase_deg = result->phase_rad * 180.0f / SIGNAL_LOCK_IN_PI_F;
    return (isfinite(result->amplitude_peak_v) && isfinite(result->phase_rad))
               ? SIGNAL_ALGORITHM_OK
               : SIGNAL_ALGORITHM_NUMERIC_ERROR;
}
