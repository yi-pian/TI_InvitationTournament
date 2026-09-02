/* 反相 OPA：由目标 -5 倍和 2 kOhm 输入电阻计算反馈电阻。 */
#include "signal_opa_inverting.h"

void opa_inverting_MinimalExample(void)
{
    signal_opa_config_t config;
    float feedback_resistor_ohm = 0.0f;
    signal_result_t result = SignalOPAInverting_MakeConfig(
        -5.0f, 2000.0f, 1.65f, &config, &feedback_resistor_ohm);

    /* 成功时反馈电阻为 10 kOhm；该 config 仍需映射到 SysConfig。 */
    if (result != SIGNAL_RESULT_OK) {
        return;
    }
}
