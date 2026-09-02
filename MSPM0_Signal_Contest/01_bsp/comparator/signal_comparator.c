#include "signal_comparator.h"

#include <stddef.h>

signal_result_t SignalComparator_ValidateConfig(
    const signal_comparator_config_t *config, float supply_voltage_v)
{
    if ((config == NULL) || !(supply_voltage_v > 0.0f) ||
        (config->threshold_v < 0.0f) ||
        (config->threshold_v > supply_voltage_v) ||
        (config->hysteresis_v < 0.0f) ||
        (config->hysteresis_v > supply_voltage_v)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalComparator_Apply(const signal_comparator_t *comparator,
    const signal_comparator_config_t *config, float supply_voltage_v)
{
    signal_result_t result = SignalComparator_ValidateConfig(config,
        supply_voltage_v);
    if (result != SIGNAL_RESULT_OK) {
        return result;
    }
    if ((comparator == NULL) || (comparator->apply == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return comparator->apply(comparator->context, config);
}

signal_module_status_t SignalComparator_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
