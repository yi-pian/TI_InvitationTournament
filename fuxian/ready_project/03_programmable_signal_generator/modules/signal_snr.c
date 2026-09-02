#include "signal_snr.h"

#include <math.h>
#include <stddef.h>

static int SignalSNR_IsExcluded(uint32_t bin, const signal_snr_config_t *config)
{
    uint32_t range_index;
    for (range_index = 0U; range_index < config->excluded_range_count;
         ++range_index)
    {
        if ((bin >= config->excluded_ranges[range_index].start_bin) &&
            (bin <= config->excluded_ranges[range_index].end_bin))
        {
            return 1;
        }
    }
    return 0;
}

signal_algorithm_status_t SignalSNR_Process(
    const float *magnitude,
    uint32_t bin_count,
    const signal_snr_config_t *config,
    signal_snr_result_t *result)
{
    uint32_t bin;
    float signal_energy = 0.0f;
    float noise_energy = 0.0f;
    uint32_t noise_bin_count = 0U;

    if ((magnitude == NULL) || (config == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((bin_count == 0U) ||
        (config->signal_start_bin > config->signal_end_bin) ||
        (config->analysis_start_bin > config->analysis_end_bin) ||
        (config->signal_start_bin < config->analysis_start_bin) ||
        (config->signal_end_bin > config->analysis_end_bin) ||
        (config->analysis_end_bin >= bin_count))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    if ((config->excluded_range_count > 0U) &&
        (config->excluded_ranges == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (bin = 0U; bin < config->excluded_range_count; ++bin)
    {
        if ((config->excluded_ranges[bin].start_bin >
             config->excluded_ranges[bin].end_bin) ||
            (config->excluded_ranges[bin].end_bin >= bin_count))
        {
            return SIGNAL_ALGORITHM_OUT_OF_RANGE;
        }
    }

    for (bin = config->analysis_start_bin;
         bin <= config->analysis_end_bin; ++bin)
    {
        float value = magnitude[bin];
        if (!isfinite(value) || (value < 0.0f))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        if ((bin >= config->signal_start_bin) &&
            (bin <= config->signal_end_bin))
        {
            signal_energy += value * value;
        }
        else if (!SignalSNR_IsExcluded(bin, config))
        {
            noise_energy += value * value;
            ++noise_bin_count;
        }
    }
    if ((signal_energy <= 0.0f) || (noise_energy <= 0.0f) ||
        (noise_bin_count == 0U))
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }
    result->signal_energy = signal_energy;
    result->noise_energy = noise_energy;
    result->snr_power_ratio = signal_energy / noise_energy;
    result->snr_db = 10.0f * log10f(result->snr_power_ratio);
    result->noise_bin_count = noise_bin_count;
    return (isfinite(result->snr_db) && isfinite(result->snr_power_ratio))
               ? SIGNAL_ALGORITHM_OK
               : SIGNAL_ALGORITHM_NUMERIC_ERROR;
}
