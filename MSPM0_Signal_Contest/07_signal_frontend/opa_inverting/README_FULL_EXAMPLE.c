/* 反相 OPA：展示当前头文件的全部公开 API。 */
#include "signal_opa_inverting.h"

void opa_inverting_FullExample(void)
{
    signal_opa_config_t config;
    float feedback_resistor_ohm = 0.0f;
    signal_module_status_t status = SignalOPAInverting_GetModuleStatus();
    signal_result_t result = SignalOPAInverting_MakeConfig(
        -5.0f, 2000.0f, 1.65f, &config, &feedback_resistor_ohm);

    /* 先确认参数计算成功，再按当前板子的 SysConfig 配真实 OPA。 */
    if ((status != MODULE_STATUS_BUILD_VERIFIED) ||
        (result != SIGNAL_RESULT_OK)) {
        return;
    }
}
