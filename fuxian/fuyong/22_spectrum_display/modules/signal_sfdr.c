#include "signal_sfdr.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalSFDR_Process(
    const float *magnitude,
    uint32_t bin_count,
    const signal_sfdr_config_t *config,
    signal_sfdr_result_t *result)
{
    uint32_t bin;
    uint32_t main_peak_bin = 0U;
    uint32_t spur_peak_bin = 0U;
    float main_peak = -1.0f;
    float spur_peak = -1.0f;

    if ((magnitude == NULL) || (config == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((bin_count == 0U) ||
        (config->main_start_bin > config->main_end_bin) ||
        (config->analysis_start_bin > config->analysis_end_bin) ||
        (config->main_start_bin < config->analysis_start_bin) ||
        (config->main_end_bin > config->analysis_end_bin) ||
        (config->analysis_end_bin >= bin_count))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }

    for (bin = config->analysis_start_bin;
         bin <= config->analysis_end_bin; ++bin)
    {
        float value = magnitude[bin];
        if (!isfinite(value) || (value < 0.0f))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        if ((bin >= config->main_start_bin) &&
            (bin <= config->main_end_bin))
        {
            if (value > main_peak)
            {
                main_peak = value;
                main_peak_bin = bin;
            }
        }
        else if (value > spur_peak)
        {
            spur_peak = value;
            spur_peak_bin = bin;
        }
    }
    if ((main_peak <= 0.0f) || (spur_peak <= 0.0f))
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }
    result->main_peak_bin = main_peak_bin;
    result->spur_peak_bin = spur_peak_bin;
    result->main_peak_magnitude = main_peak;
    result->spur_peak_magnitude = spur_peak;
    result->sfdr_ratio = main_peak / spur_peak;
    result->sfdr_db = 20.0f * log10f(result->sfdr_ratio);
    return (isfinite(result->sfdr_ratio) && isfinite(result->sfdr_db))
               ? SIGNAL_ALGORITHM_OK
               : SIGNAL_ALGORITHM_NUMERIC_ERROR;
}
