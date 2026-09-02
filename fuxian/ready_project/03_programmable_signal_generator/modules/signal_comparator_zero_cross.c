#include "signal_comparator_zero_cross.h"

#include <stddef.h>

signal_result_t SignalComparatorZeroCross_MakeConfig(float virtual_ground_v,
    float hysteresis_v, signal_comparator_config_t *config)
{
    if ((config == NULL) || (virtual_ground_v < 0.0f) ||
        (hysteresis_v < 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    config->threshold_v = virtual_ground_v;
    config->hysteresis_v = hysteresis_v;
    config->invert_output = false;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalComparatorZeroCross_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
