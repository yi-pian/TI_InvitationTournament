#include "signal_czt.h"

#include <float.h>
#include <math.h>
#include <stddef.h>

#define SIGNAL_CZT_TWO_PI_F (6.2831853071795864769f)

signal_algorithm_status_t SignalCZT_UnitCircleRealDirect(
    const float *samples,
    uint32_t sample_count,
    float sample_rate_hz,
    float start_frequency_hz,
    float frequency_step_hz,
    signal_complex_f32_t *output,
    uint32_t output_count,
    uint32_t output_capacity)
{
    uint32_t sample_index;
    uint32_t output_index;
    float final_frequency;
    float maximum_sample = 0.0f;

    if ((samples == NULL) || (output == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((sample_count == 0U) || (output_count == 0U))
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (output_capacity < output_count)
    {
        return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
    }
    if (!isfinite(sample_rate_hz) || !isfinite(start_frequency_hz) ||
        !isfinite(frequency_step_hz) || (sample_rate_hz <= 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    final_frequency = start_frequency_hz +
        ((float)(output_count - 1U) * frequency_step_hz);
    if ((fabsf(start_frequency_hz) > (0.5f * sample_rate_hz)) ||
        (fabsf(final_frequency) > (0.5f * sample_rate_hz)))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    for (sample_index = 0U; sample_index < sample_count; ++sample_index)
    {
        float magnitude;
        if (!isfinite(samples[sample_index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        magnitude = fabsf(samples[sample_index]);
        if (magnitude > maximum_sample)
        {
            maximum_sample = magnitude;
        }
    }
    if (maximum_sample > (FLT_MAX / (2.0f * (float)sample_count)))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }

    for (output_index = 0U; output_index < output_count; ++output_index)
    {
        float frequency = start_frequency_hz +
                          ((float)output_index * frequency_step_hz);
        float angle = -SIGNAL_CZT_TWO_PI_F * frequency / sample_rate_hz;
        float step_real = cosf(angle);
        float step_imag = sinf(angle);
        float phasor_real = 1.0f;
        float phasor_imag = 0.0f;
        float sum_real = 0.0f;
        float sum_imag = 0.0f;
        for (sample_index = 0U; sample_index < sample_count; ++sample_index)
        {
            float next_real;
            sum_real += samples[sample_index] * phasor_real;
            sum_imag += samples[sample_index] * phasor_imag;
            next_real = (phasor_real * step_real) -
                        (phasor_imag * step_imag);
            phasor_imag = (phasor_real * step_imag) +
                          (phasor_imag * step_real);
            phasor_real = next_real;
        }
        output[output_index].real = sum_real;
        output[output_index].imag = sum_imag;
    }
    return SIGNAL_ALGORITHM_OK;
}
