#include "signal_gpamp.h"

#include <stddef.h>

signal_result_t SignalGPAMP_ValidateConfig(const signal_gpamp_config_t *config)
{
    if ((config == NULL) || !(config->requested_gain > 0.0f) ||
        (config->bias_voltage_v < 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalGPAMP_Apply(const signal_gpamp_t *gpamp,
    const signal_gpamp_config_t *config)
{
    signal_result_t result = SignalGPAMP_ValidateConfig(config);
    if (result != SIGNAL_RESULT_OK) {
        return result;
    }
    if ((gpamp == NULL) || (gpamp->apply == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return gpamp->apply(gpamp->context, config);
}

signal_module_status_t SignalGPAMP_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
