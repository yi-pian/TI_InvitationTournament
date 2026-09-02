/* median_filter 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_median_filter.h"

void median_filter_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalmedianfilter_process_arg0[16] = {0};
    static float signalmedianfilter_process_arg1[16] = {0};
    static uint32_t signalmedianfilter_process_arg2 = 0U;
    static uint32_t signalmedianfilter_process_arg3 = 0U;
    static float signalmedianfilter_process_arg4[16] = {0};
    static uint32_t signalmedianfilter_process_arg5 = 0U;
    /* ===== 最小入口：SignalMedianFilter_Process ===== */
    (void)SignalMedianFilter_Process(signalmedianfilter_process_arg0, signalmedianfilter_process_arg1, signalmedianfilter_process_arg2, signalmedianfilter_process_arg3, signalmedianfilter_process_arg4, signalmedianfilter_process_arg5);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

