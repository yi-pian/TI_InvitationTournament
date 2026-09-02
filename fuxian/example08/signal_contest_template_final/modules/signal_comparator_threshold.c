#include "signal_comparator_threshold.h"

#include <stddef.h>

signal_result_t SignalComparatorThreshold_MakeConfig(float threshold_v,
    float hysteresis_v, bool invert_output,
    signal_comparator_config_t *config)
{
    if ((config == NULL) || (threshold_v < 0.0f) || (hysteresis_v < 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    config->threshold_v = threshold_v;
    config->hysteresis_v = hysteresis_v;
    config->invert_output = invert_output;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalComparatorThreshold_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
