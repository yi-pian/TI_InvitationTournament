/* coherent_sampling 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_coherent_sampling.h"

void coherent_sampling_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalcoherentsampling_findnearest_arg0 = 0.0f;
    static float signalcoherentsampling_findnearest_arg1 = 0.0f;
    static uint32_t signalcoherentsampling_findnearest_arg2 = 0U;
    static uint32_t signalcoherentsampling_findnearest_arg3 = 0U;
    static uint32_t signalcoherentsampling_findnearest_arg4 = 0U;
    static bool signalcoherentsampling_findnearest_arg5 = false;
    static signal_coherent_sampling_result_t signalcoherentsampling_findnearest_arg6 = {0};
    /* ===== 最小入口：SignalCoherentSampling_FindNearest ===== */
    (void)SignalCoherentSampling_FindNearest(signalcoherentsampling_findnearest_arg0, signalcoherentsampling_findnearest_arg1, signalcoherentsampling_findnearest_arg2, signalcoherentsampling_findnearest_arg3, signalcoherentsampling_findnearest_arg4, signalcoherentsampling_findnearest_arg5, &signalcoherentsampling_findnearest_arg6);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

