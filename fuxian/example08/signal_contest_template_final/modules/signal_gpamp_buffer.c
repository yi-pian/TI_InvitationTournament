#include "signal_gpamp_buffer.h"

#include <stddef.h>

signal_result_t SignalGPAMPBuffer_MakeConfig(float bias_voltage_v,
    signal_gpamp_config_t *config)
{
    if ((config == NULL) || (bias_voltage_v < 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    config->requested_gain = 1.0f;
    config->bias_voltage_v = bias_voltage_v;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalGPAMPBuffer_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
