#include "signal_vca820_gain_control.h"

#include <math.h>

signal_algorithm_status_t SignalVCA820_TargetVppToDACCode(
    float target_vpp_v,
    const signal_vca820_gain_config_t *config,
    uint16_t *dac_code)
{
    float required_gain;
    float control_voltage_v;
    float gain_db;
    float code;

    if ((config == 0) || (dac_code == 0) ||
        (target_vpp_v <= 0.0f) || (config->dds_vpp_v <= 0.0f) ||
        (config->gain_max <= 0.0f) || (config->dac_reference_v <= 0.0f) ||
        (config->dac_full_scale_code <= 0.0f) ||
        (config->control_max_v < config->control_min_v)) {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }

    required_gain = target_vpp_v / config->dds_vpp_v;
    if (required_gain >= config->gain_max) {
        control_voltage_v = config->control_max_v;
    } else {
        gain_db = 20.0f * log10f(required_gain);
        control_voltage_v =
            (gain_db / -40.0f - 1.0f) / -0.953f;
    }

    if (control_voltage_v < config->control_min_v) {
        control_voltage_v = config->control_min_v;
    }
    if (control_voltage_v > config->control_max_v) {
        control_voltage_v = config->control_max_v;
    }

    code = control_voltage_v / config->dac_reference_v *
        config->dac_full_scale_code;
    if (code > 65535.0f) {
        code = 65535.0f;
    }
    *dac_code = (uint16_t)(code + 0.5f);
    return SIGNAL_ALGORITHM_OK;
}
