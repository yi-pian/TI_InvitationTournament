#include "signal_vca820_gain_control.h"

void VCA820_FullExample(float target_vpp_v)
{
    const signal_vca820_gain_config_t config = {
        .dds_vpp_v = 0.35f, .gain_max = 10.0f,
        .vctrl0_v = 0.85f, .vctrl_slope_v = 0.09f,
        .control_min_v = 0.0f, .control_max_v = 2.0f,
        .dac_reference_v = 3.3f, .dac_full_scale_code = 4095.0f
    };
    uint16_t dac_code = 0U;
    signal_algorithm_status_t status =
        SignalVCA820_TargetVppToDACCode(target_vpp_v, &config, &dac_code);
    (void)status;
    (void)dac_code;
}
