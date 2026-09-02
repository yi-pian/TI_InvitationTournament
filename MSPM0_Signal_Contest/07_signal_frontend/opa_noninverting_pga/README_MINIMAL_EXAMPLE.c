/* 同相 OPA：由目标 11 倍和 1 kOhm 接地电阻计算反馈电阻。 */
#include "signal_opa_noninverting_pga.h"

void opa_noninverting_pga_MinimalExample(void)
{
    signal_opa_config_t config;
    float feedback_resistor_ohm = 0.0f;
    signal_result_t result = SignalOPANoninvertingPGA_MakeConfig(
        11.0f, 1000.0f, 1.65f, &config, &feedback_resistor_ohm);

    /* 成功时反馈电阻为 10 kOhm；实际增益档仍以 SysConfig 为准。 */
    if (result != SIGNAL_RESULT_OK) {
        return;
    }
}
