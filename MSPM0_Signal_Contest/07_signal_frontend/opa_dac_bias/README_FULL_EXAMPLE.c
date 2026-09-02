/* OPA + DAC 偏置：展示当前头文件的全部公开 API。 */
#include "signal_opa_dac_bias.h"

void opa_dac_bias_FullExample(void)
{
    float output_voltage_v = 0.0f;
    signal_module_status_t status = SignalOPADACBias_GetModuleStatus();
    signal_result_t result = SignalOPADACBias_Calculate(
        0.15f, 4.0f, 1.65f, &output_voltage_v);

    /* status 是软件模块状态；result 成功后才能使用 output_voltage_v。 */
    if ((status != MODULE_STATUS_BUILD_VERIFIED) ||
        (result != SIGNAL_RESULT_OK)) {
        return;
    }
}
