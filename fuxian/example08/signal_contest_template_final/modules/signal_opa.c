#include "signal_opa.h"

#include <stddef.h>

signal_result_t SignalOPA_CalculateGain(const signal_opa_config_t *config,
    float *gain)
{
    if ((config == NULL) || (gain == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    switch (config->mode) {
        case SIGNAL_OPA_MODE_BUFFER:
            *gain = 1.0f;
            return SIGNAL_RESULT_OK;
        case SIGNAL_OPA_MODE_NONINVERTING:
            if (!(config->resistor_input_ohm > 0.0f) ||
                (config->resistor_feedback_ohm < 0.0f)) {
                return SIGNAL_RESULT_INVALID_ARGUMENT;
            }
            *gain = 1.0f + config->resistor_feedback_ohm /
                config->resistor_input_ohm;
            return SIGNAL_RESULT_OK;
        case SIGNAL_OPA_MODE_INVERTING:
            if (!(config->resistor_input_ohm > 0.0f) ||
                (config->resistor_feedback_ohm < 0.0f)) {
                return SIGNAL_RESULT_INVALID_ARGUMENT;
            }
            *gain = -config->resistor_feedback_ohm /
                config->resistor_input_ohm;
            return SIGNAL_RESULT_OK;
        default:
            return SIGNAL_RESULT_OUT_OF_RANGE;
    }
}

signal_result_t SignalOPA_Apply(const signal_opa_t *opa,
    const signal_opa_config_t *config)
{
    float unused_gain;
    signal_result_t result = SignalOPA_CalculateGain(config, &unused_gain);
    if (result != SIGNAL_RESULT_OK) {
        return result;
    }
    if ((opa == NULL) || (opa->apply == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return opa->apply(opa->context, config);
}

signal_module_status_t SignalOPA_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
