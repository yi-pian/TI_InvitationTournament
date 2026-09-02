/* harmonic 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_harmonic.h"

void harmonic_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalharmonic_process_arg0[16] = {0};
    static uint32_t signalharmonic_process_arg1 = 0U;
    static float signalharmonic_process_arg2 = 0.0f;
    static uint32_t signalharmonic_process_arg3 = 0U;
    static signal_harmonic_config_t signalharmonic_process_arg4 = {0};
    static signal_harmonic_result_t signalharmonic_process_arg5 = {0};
    /* ===== 调用 SignalHarmonic_Process：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalHarmonic_Process(signalharmonic_process_arg0, signalharmonic_process_arg1, signalharmonic_process_arg2, signalharmonic_process_arg3, &signalharmonic_process_arg4, &signalharmonic_process_arg5);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

