/* robust_rms 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_robust_rms.h"

void robust_rms_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalrobustrms_process_arg0[16] = {0};
    static uint32_t signalrobustrms_process_arg1 = 0U;
    static signal_robust_rms_config_t signalrobustrms_process_arg2 = {0};
    static float signalrobustrms_process_arg3[16] = {0};
    static uint32_t signalrobustrms_process_arg4 = 0U;
    static signal_robust_rms_result_t signalrobustrms_process_arg5 = {0};
    /* ===== 调用 SignalRobustRMS_Process：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalRobustRMS_Process(signalrobustrms_process_arg0, signalrobustrms_process_arg1, &signalrobustrms_process_arg2, signalrobustrms_process_arg3, signalrobustrms_process_arg4, &signalrobustrms_process_arg5);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

