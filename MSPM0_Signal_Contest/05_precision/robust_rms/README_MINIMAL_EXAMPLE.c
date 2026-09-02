/* robust_rms 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_robust_rms.h"

void robust_rms_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalrobustrms_process_arg0[16] = {0};
    static uint32_t signalrobustrms_process_arg1 = 0U;
    static signal_robust_rms_config_t signalrobustrms_process_arg2 = {0};
    static float signalrobustrms_process_arg3[16] = {0};
    static uint32_t signalrobustrms_process_arg4 = 0U;
    static signal_robust_rms_result_t signalrobustrms_process_arg5 = {0};
    /* ===== 最小入口：SignalRobustRMS_Process ===== */
    (void)SignalRobustRMS_Process(signalrobustrms_process_arg0, signalrobustrms_process_arg1, &signalrobustrms_process_arg2, signalrobustrms_process_arg3, signalrobustrms_process_arg4, &signalrobustrms_process_arg5);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

