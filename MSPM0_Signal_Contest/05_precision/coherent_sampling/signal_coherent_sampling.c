#include "signal_coherent_sampling.h"

#include <float.h>
#include <math.h>
#include <stddef.h>

uint32_t SignalCoherentSampling_GCDU32(uint32_t a, uint32_t b)
{
    while (b != 0U)
    {
        uint32_t remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

signal_algorithm_status_t SignalCoherentSampling_FindNearest(
    float desired_frequency_hz,
    float sample_rate_hz,
    uint32_t sample_count,
    uint32_t minimum_cycles,
    uint32_t maximum_cycles,
    bool require_coprime,
    signal_coherent_sampling_result_t *result)
{
    signal_coherent_sampling_result_t temporary;
    uint32_t cycles;
    uint32_t best_cycles = 0U;
    float best_absolute_error = FLT_MAX;

    if (result == NULL)
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (sample_count < 2U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(desired_frequency_hz) || !isfinite(sample_rate_hz) ||
        (desired_frequency_hz <= 0.0f) || (sample_rate_hz <= 0.0f) ||
        (desired_frequency_hz >= (0.5f * sample_rate_hz)))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    if ((minimum_cycles == 0U) || (minimum_cycles > maximum_cycles) ||
        (maximum_cycles > (sample_count / 2U)))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }

    for (cycles = minimum_cycles; cycles <= maximum_cycles; ++cycles)
    {
        float candidate_frequency;
        float candidate_error;
        if (require_coprime &&
            (SignalCoherentSampling_GCDU32(cycles, sample_count) != 1U))
        {
            continue;
        }
        candidate_frequency = ((float)cycles * sample_rate_hz) /
                              (float)sample_count;
        candidate_error = fabsf(candidate_frequency - desired_frequency_hz);
        if ((candidate_error < best_absolute_error) ||
            ((candidate_error == best_absolute_error) && (cycles < best_cycles)))
        {
            best_absolute_error = candidate_error;
            best_cycles = cycles;
        }
    }
    if (best_cycles == 0U)
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }
    temporary.cycles_per_record = best_cycles;
    temporary.samples_per_record = sample_count;
    temporary.cycle_sample_gcd =
        SignalCoherentSampling_GCDU32(best_cycles, sample_count);
    temporary.coherent_frequency_hz = ((float)best_cycles * sample_rate_hz) /
                                      (float)sample_count;
    temporary.frequency_error_hz = temporary.coherent_frequency_hz -
                                   desired_frequency_hz;
    temporary.absolute_error_hz = fabsf(temporary.frequency_error_hz);
    temporary.relative_error_ppm = 1.0e6f * temporary.frequency_error_hz /
                                   desired_frequency_hz;
    *result = temporary;
    return SIGNAL_ALGORITHM_OK;
}
