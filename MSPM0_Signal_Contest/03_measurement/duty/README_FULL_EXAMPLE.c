/* duty 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_duty.h"

void duty_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalduty_process_arg0[16] = {0};
    static uint32_t signalduty_process_arg1 = 0U;
    static float signalduty_process_arg2 = 0.0f;
    static signal_duty_config_t signalduty_process_arg3 = {0};
    static signal_duty_result_t signalduty_process_arg4 = {0};
    /* ===== 调用 SignalDuty_Process：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDuty_Process(signalduty_process_arg0, signalduty_process_arg1, signalduty_process_arg2, &signalduty_process_arg3, &signalduty_process_arg4);

    static signal_duty_config_t signalduty_getdefaultconfig_arg0 = {0};
    /* ===== 调用 SignalDuty_GetDefaultConfig：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDuty_GetDefaultConfig(&signalduty_getdefaultconfig_arg0);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

