#include "signal_opa_noninverting_pga.h"

#include <stddef.h>

signal_result_t SignalOPANoninvertingPGA_MakeConfig(float requested_gain,
    float resistor_to_ground_ohm, float bias_voltage_v,
    signal_opa_config_t *config, float *feedback_resistor_ohm)
{
    if ((config == NULL) || (feedback_resistor_ohm == NULL) ||
        (requested_gain < 1.0f) || !(resistor_to_ground_ohm > 0.0f) ||
        (bias_voltage_v < 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *feedback_resistor_ohm = (requested_gain - 1.0f) *
        resistor_to_ground_ohm;
    config->mode = SIGNAL_OPA_MODE_NONINVERTING;
    config->resistor_feedback_ohm = *feedback_resistor_ohm;
    config->resistor_input_ohm = resistor_to_ground_ohm;
    config->bias_voltage_v = bias_voltage_v;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalOPANoninvertingPGA_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
