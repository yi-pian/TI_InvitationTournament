#include "signal_window.h"

#include <math.h>
#include <stddef.h>

#define SIGNAL_WINDOW_PI_F 3.14159265358979323846f

static float SignalWindow_Coefficient(
    uint32_t index,
    uint32_t count,
    signal_window_type_t type)
{
    float angle = (2.0f * SIGNAL_WINDOW_PI_F * (float)index) /
                  (float)(count - 1U);

    switch (type)
    {
        case SIGNAL_WINDOW_RECTANGULAR:
            return 1.0f;
        case SIGNAL_WINDOW_HANN:
            return 0.5f - (0.5f * cosf(angle));
        case SIGNAL_WINDOW_HAMMING:
            return 0.54f - (0.46f * cosf(angle));
        case SIGNAL_WINDOW_BLACKMAN:
            return 0.42f - (0.5f * cosf(angle)) +
                   (0.08f * cosf(2.0f * angle));
        default:
            return NAN;
    }
}

signal_algorithm_status_t SignalWindow_Apply(
    const float *input_samples,
    float *output_samples,
    uint32_t count,
    signal_window_type_t type,
    signal_window_result_t *result)
{
    uint32_t index;
    float coefficient_sum = 0.0f;
    float coefficient_square_sum = 0.0f;

    if ((input_samples == NULL) || (output_samples == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count < 2U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (type > SIGNAL_WINDOW_BLACKMAN)
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(input_samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }

    for (index = 0U; index < count; ++index)
    {
        float coefficient = SignalWindow_Coefficient(index, count, type);
        output_samples[index] = input_samples[index] * coefficient;
        coefficient_sum += coefficient;
        coefficient_square_sum += coefficient * coefficient;
    }
    result->coherent_gain = coefficient_sum / (float)count;
    result->power_gain = coefficient_square_sum / (float)count;
    if (!isfinite(result->coherent_gain) || !isfinite(result->power_gain) ||
        (result->coherent_gain <= 0.0f) || (result->power_gain <= 0.0f))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    return SIGNAL_ALGORITHM_OK;
}
