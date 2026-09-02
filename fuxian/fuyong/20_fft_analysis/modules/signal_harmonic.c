#include "signal_harmonic.h"

#include <math.h>
#include <stddef.h>

#include "signal_multi_bin_energy.h"

signal_algorithm_status_t SignalHarmonic_Process(
    const float *magnitude,
    uint32_t bin_count,
    float sample_rate_hz,
    uint32_t fft_size,
    const signal_harmonic_config_t *config,
    signal_harmonic_result_t *result)
{
    uint32_t order;
    uint32_t previous_end_bin = 0U;

    if ((magnitude == NULL) || (config == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((fft_size < 2U) || (bin_count != ((fft_size / 2U) + 1U)))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    if (!isfinite(sample_rate_hz) || (sample_rate_hz <= 0.0f) ||
        !isfinite(config->fundamental_frequency_hz) ||
        (config->fundamental_frequency_hz <= 0.0f) ||
        (config->first_order == 0U) ||
        (config->first_order > config->last_order) ||
        (config->last_order > SIGNAL_HARMONIC_MAX_ORDER))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }

    result->first_order = config->first_order;
    result->last_order = config->last_order;
    for (order = config->first_order; order <= config->last_order; ++order)
    {
        signal_harmonic_item_t *item = &result->items[order];
        signal_multi_bin_energy_result_t energy_result;
        signal_algorithm_status_t status;
        float target_frequency_hz = config->fundamental_frequency_hz * (float)order;
        float target_bin = target_frequency_hz * (float)fft_size / sample_rate_hz;
        uint32_t center_bin;

        if (!isfinite(target_frequency_hz) ||
            (target_frequency_hz > (0.5f * sample_rate_hz)) ||
            !isfinite(target_bin) || (target_bin >= (float)bin_count))
        {
            return SIGNAL_ALGORITHM_OUT_OF_RANGE;
        }
        center_bin = (uint32_t)(target_bin + 0.5f);
        if (center_bin >= bin_count)
        {
            return SIGNAL_ALGORITHM_OUT_OF_RANGE;
        }
        status = SignalMultiBinEnergy_Process(
            magnitude, bin_count, center_bin, config->radius_bins,
            &energy_result);
        if (status != SIGNAL_ALGORITHM_OK)
        {
            return status;
        }
        if ((order > config->first_order) &&
            (energy_result.start_bin <= previous_end_bin))
        {
            return SIGNAL_ALGORITHM_OUT_OF_RANGE;
        }
        previous_end_bin = energy_result.end_bin;
        item->order = order;
        item->target_frequency_hz = target_frequency_hz;
        item->target_fractional_bin = target_bin;
        item->center_bin = center_bin;
        item->start_bin = energy_result.start_bin;
        item->end_bin = energy_result.end_bin;
        item->energy = energy_result.energy;
        item->root_sum_square = energy_result.root_sum_square;
    }
    return SIGNAL_ALGORITHM_OK;
}
