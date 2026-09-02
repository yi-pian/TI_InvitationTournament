/* coherent_sampling 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_coherent_sampling.h"

void coherent_sampling_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalcoherentsampling_findnearest_arg0 = 0.0f;
    static float signalcoherentsampling_findnearest_arg1 = 0.0f;
    static uint32_t signalcoherentsampling_findnearest_arg2 = 0U;
    static uint32_t signalcoherentsampling_findnearest_arg3 = 0U;
    static uint32_t signalcoherentsampling_findnearest_arg4 = 0U;
    static bool signalcoherentsampling_findnearest_arg5 = false;
    static signal_coherent_sampling_result_t signalcoherentsampling_findnearest_arg6 = {0};
    /* ===== 调用 SignalCoherentSampling_FindNearest：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalCoherentSampling_FindNearest(signalcoherentsampling_findnearest_arg0, signalcoherentsampling_findnearest_arg1, signalcoherentsampling_findnearest_arg2, signalcoherentsampling_findnearest_arg3, signalcoherentsampling_findnearest_arg4, signalcoherentsampling_findnearest_arg5, &signalcoherentsampling_findnearest_arg6);

    static uint32_t signalcoherentsampling_gcdu32_arg0 = 0U;
    static uint32_t signalcoherentsampling_gcdu32_arg1 = 0U;
    /* ===== 调用 SignalCoherentSampling_GCDU32：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalCoherentSampling_GCDU32(signalcoherentsampling_gcdu32_arg0, signalcoherentsampling_gcdu32_arg1);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

