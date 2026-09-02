#ifndef SIGNAL_VCA820_GAIN_CONTROL_H
#define SIGNAL_VCA820_GAIN_CONTROL_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct {
    float dds_vpp_v;
    float gain_max;
    float vctrl0_v;
    float vctrl_slope_v;
    float control_min_v;
    float control_max_v;
    float dac_reference_v;
    float dac_full_scale_code;
} signal_vca820_gain_config_t;

signal_algorithm_status_t SignalVCA820_TargetVppToDACCode(
    float target_vpp_v,
    const signal_vca820_gain_config_t *config,
    uint16_t *dac_code);

#endif
