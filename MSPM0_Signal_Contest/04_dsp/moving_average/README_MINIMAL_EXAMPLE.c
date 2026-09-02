/* moving_average 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_moving_average.h"

void moving_average_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalmovingaverage_process_arg0[16] = {0};
    static float signalmovingaverage_process_arg1[16] = {0};
    static uint32_t signalmovingaverage_process_arg2 = 0U;
    static uint32_t signalmovingaverage_process_arg3 = 0U;
    /* ===== 最小入口：SignalMovingAverage_Process ===== */
    (void)SignalMovingAverage_Process(signalmovingaverage_process_arg0, signalmovingaverage_process_arg1, signalmovingaverage_process_arg2, signalmovingaverage_process_arg3);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

