/* phase 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_phase.h"

void phase_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalphase_fromcorrelationlag_arg0 = 0.0f;
    static float signalphase_fromcorrelationlag_arg1 = 0.0f;
    static signal_phase_result_t signalphase_fromcorrelationlag_arg2 = {0};
    /* ===== 调用 SignalPhase_FromCorrelationLag：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalPhase_FromCorrelationLag(signalphase_fromcorrelationlag_arg0, signalphase_fromcorrelationlag_arg1, &signalphase_fromcorrelationlag_arg2);

    static signal_complex_f32_t signalphase_fromfftbin_arg0 = {0};
    static signal_complex_f32_t signalphase_fromfftbin_arg1 = {0};
    static uint32_t signalphase_fromfftbin_arg2 = 0U;
    static uint32_t signalphase_fromfftbin_arg3 = 0U;
    static signal_phase_result_t signalphase_fromfftbin_arg4 = {0};
    /* ===== 调用 SignalPhase_FromFFTBin：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalPhase_FromFFTBin(&signalphase_fromfftbin_arg0, &signalphase_fromfftbin_arg1, signalphase_fromfftbin_arg2, signalphase_fromfftbin_arg3, &signalphase_fromfftbin_arg4);

    static float signalphase_fromzerocross_arg0 = 0.0f;
    static float signalphase_fromzerocross_arg1 = 0.0f;
    static float signalphase_fromzerocross_arg2 = 0.0f;
    static signal_phase_result_t signalphase_fromzerocross_arg3 = {0};
    /* ===== 调用 SignalPhase_FromZeroCross：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalPhase_FromZeroCross(signalphase_fromzerocross_arg0, signalphase_fromzerocross_arg1, signalphase_fromzerocross_arg2, &signalphase_fromzerocross_arg3);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

