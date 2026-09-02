/* robust_peak_to_peak 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_robust_peak_to_peak.h"

void robust_peak_to_peak_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalrobustpeaktopeak_process_arg0[16] = {0};
    static uint32_t signalrobustpeaktopeak_process_arg1 = 0U;
    static signal_robust_peak_to_peak_config_t signalrobustpeaktopeak_process_arg2 = {0};
    static float signalrobustpeaktopeak_process_arg3[16] = {0};
    static uint32_t signalrobustpeaktopeak_process_arg4 = 0U;
    static signal_robust_peak_to_peak_result_t signalrobustpeaktopeak_process_arg5 = {0};
    /* ===== 最小入口：SignalRobustPeakToPeak_Process ===== */
    (void)SignalRobustPeakToPeak_Process(signalrobustpeaktopeak_process_arg0, signalrobustpeaktopeak_process_arg1, &signalrobustpeaktopeak_process_arg2, signalrobustpeaktopeak_process_arg3, signalrobustpeaktopeak_process_arg4, &signalrobustpeaktopeak_process_arg5);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

