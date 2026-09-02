#include "signal_frequency_sweep.h"

#include <math.h>
#include <stddef.h>

signal_result_t SignalFrequencySweep_Generate(
    const signal_frequency_sweep_config_t *config, float *frequencies_hz,
    size_t capacity)
{
    size_t index;
    if ((config == NULL) || (frequencies_hz == NULL) ||
        !(config->start_hz > 0.0f) || !(config->stop_hz > config->start_hz) ||
        (config->point_count < 2U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (capacity < config->point_count) {
        return SIGNAL_RESULT_INSUFFICIENT_BUFFER;
    }
    for (index = 0U; index < config->point_count; ++index) {
        float fraction = (float) index / (float) (config->point_count - 1U);
        frequencies_hz[index] = config->logarithmic ?
            config->start_hz * powf(config->stop_hz / config->start_hz,
                fraction) :
            config->start_hz + (config->stop_hz - config->start_hz) * fraction;
    }
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalFrequencySweep_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
