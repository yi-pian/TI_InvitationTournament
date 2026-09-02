/* 同相 OPA：展示当前头文件的全部公开 API。 */
#include "signal_opa_noninverting_pga.h"

void opa_noninverting_pga_FullExample(void)
{
    signal_opa_config_t config;
    float feedback_resistor_ohm = 0.0f;
    signal_module_status_t status = SignalOPANoninvertingPGA_GetModuleStatus();
    signal_result_t result = SignalOPANoninvertingPGA_MakeConfig(
        11.0f, 1000.0f, 1.65f, &config, &feedback_resistor_ohm);

    /* 确认软件参数合法后，才能继续做真实硬件配置和量程验证。 */
    if ((status != MODULE_STATUS_BUILD_VERIFIED) ||
        (result != SIGNAL_RESULT_OK)) {
        return;
    }
}
