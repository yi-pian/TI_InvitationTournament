#include "signal_gpamp_gain.h"

#include <stddef.h>

signal_result_t SignalGPAMPGain_MakeConfig(float requested_gain,
    float bias_voltage_v, signal_gpamp_config_t *config)
{
    if ((config == NULL) || !(requested_gain > 0.0f) ||
        (bias_voltage_v < 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    config->requested_gain = requested_gain;
    config->bias_voltage_v = bias_voltage_v;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalGPAMPGain_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
