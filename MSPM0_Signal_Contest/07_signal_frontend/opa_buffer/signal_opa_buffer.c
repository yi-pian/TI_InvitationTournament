#include "signal_opa_buffer.h"

#include <stddef.h>

signal_result_t SignalOPABuffer_MakeConfig(float bias_voltage_v,
    signal_opa_config_t *config)
{
    if ((config == NULL) || (bias_voltage_v < 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    config->mode = SIGNAL_OPA_MODE_BUFFER;
    config->resistor_feedback_ohm = 0.0f;
    config->resistor_input_ohm = 0.0f;
    config->bias_voltage_v = bias_voltage_v;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalOPABuffer_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
