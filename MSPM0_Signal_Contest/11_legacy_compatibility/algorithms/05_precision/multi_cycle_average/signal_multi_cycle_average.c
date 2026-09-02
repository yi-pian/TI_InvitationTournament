#include "signal_multi_cycle_average.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalMultiCycleAverage_Process(
    const float *crossing_positions_samples,
    uint32_t crossing_count,
    float sample_rate_hz,
    signal_multi_cycle_average_result_t *result)
{
    uint32_t index;
    uint32_t cycle_count;
    float span_samples;
    float average_period_samples;

    if ((crossing_positions_samples == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (crossing_count < 2U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(sample_rate_hz) || (sample_rate_hz <= 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (!isfinite(crossing_positions_samples[0]))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }

    for (index = 1U; index < crossing_count; ++index)
    {
        if (!isfinite(crossing_positions_samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        if (crossing_positions_samples[index] <=
            crossing_positions_samples[index - 1U])
        {
            return SIGNAL_ALGORITHM_OUT_OF_RANGE;
        }
    }

    cycle_count = crossing_count - 1U;
    span_samples = crossing_positions_samples[crossing_count - 1U] -
                   crossing_positions_samples[0];
    average_period_samples = span_samples / (float)cycle_count;
    if (!isfinite(average_period_samples) || (average_period_samples <= 0.0f))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }

    result->average_period_samples = average_period_samples;
    result->frequency_hz = sample_rate_hz / average_period_samples;
    result->observation_time_s = span_samples / sample_rate_hz;
    result->cycle_count = cycle_count;
    if (!isfinite(result->frequency_hz) ||
        !isfinite(result->observation_time_s))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
    return SIGNAL_ALGORITHM_OK;
}
