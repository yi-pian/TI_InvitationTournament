/* OPA + DAC 偏置：计算一个输入点的理论输出电压。 */
#include "signal_opa_dac_bias.h"

void opa_dac_bias_MinimalExample(void)
{
    float output_voltage_v = 0.0f;
    signal_result_t result = SignalOPADACBias_Calculate(
        -0.10f, 4.0f, 1.65f, &output_voltage_v);

    /* 成功时 output_voltage_v 为 1.25 V；再用量程检查模块验证 ADC。 */
    if (result != SIGNAL_RESULT_OK) {
        return;
    }
}
