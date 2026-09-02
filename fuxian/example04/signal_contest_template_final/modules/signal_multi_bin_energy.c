#include "signal_multi_bin_energy.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalMultiBinEnergy_Process(
    const float *magnitude,
    uint32_t bin_count,
    uint32_t center_bin,
    uint32_t radius_bins,
    signal_multi_bin_energy_result_t *result)
{
    uint32_t start_bin;
    uint32_t end_bin;
    uint32_t bin;
    float energy = 0.0f;

    if ((magnitude == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((bin_count == 0U) || (center_bin >= bin_count))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    start_bin = (center_bin > radius_bins) ? (center_bin - radius_bins) : 0U;
    end_bin = center_bin + radius_bins;
    if ((end_bin < center_bin) || (end_bin >= bin_count))
    {
        end_bin = bin_count - 1U;
    }

    for (bin = start_bin; bin <= end_bin; ++bin)
    {
        float value = magnitude[bin];
        if (!isfinite(value) || (value < 0.0f))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        energy += value * value;
        if (!isfinite(energy))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }
    result->start_bin = start_bin;
    result->end_bin = end_bin;
    result->energy = energy;
    result->root_sum_square = sqrtf(energy);
    return SIGNAL_ALGORITHM_OK;
}
