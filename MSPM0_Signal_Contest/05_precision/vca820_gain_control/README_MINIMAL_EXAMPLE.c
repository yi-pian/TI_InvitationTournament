#include "signal_vca820_gain_control.h"

static const signal_vca820_gain_config_t g_vca_config = {
    .dds_vpp_v = 0.35f,
    .gain_max = 10.0f,
    .vctrl0_v = 0.85f,
    .vctrl_slope_v = 0.09f,
    .control_min_v = 0.0f,
    .control_max_v = 2.0f,
    .dac_reference_v = 3.3f,
    .dac_full_scale_code = 4095.0f
};

uint16_t App_GetVcaCode(float target_vpp_v)
{
    uint16_t code = 0U;
    (void)SignalVCA820_TargetVppToDACCode(
        target_vpp_v, &g_vca_config, &code);
    return code;
}
