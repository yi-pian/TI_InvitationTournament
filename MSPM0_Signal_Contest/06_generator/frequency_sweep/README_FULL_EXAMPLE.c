/* frequency_sweep 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_frequency_sweep.h"

void frequency_sweep_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_frequency_sweep_config_t signalfrequencysweep_generate_arg0 = {0};
    static float signalfrequencysweep_generate_arg1[16] = {0};
    static size_t signalfrequencysweep_generate_arg2 = 0U;
    /* ===== 调用 SignalFrequencySweep_Generate：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalFrequencySweep_Generate(&signalfrequencysweep_generate_arg0, signalfrequencysweep_generate_arg1, signalfrequencysweep_generate_arg2);

    /* ===== 调用 SignalFrequencySweep_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalFrequencySweep_GetModuleStatus();

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

