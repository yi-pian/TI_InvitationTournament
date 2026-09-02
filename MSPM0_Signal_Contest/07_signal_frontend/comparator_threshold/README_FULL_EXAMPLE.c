/* comparator_threshold 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_comparator_threshold.h"

void comparator_threshold_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalcomparatorthreshold_makeconfig_arg0 = 0.0f;
    static float signalcomparatorthreshold_makeconfig_arg1 = 0.0f;
    static bool signalcomparatorthreshold_makeconfig_arg2 = false;
    static signal_comparator_config_t signalcomparatorthreshold_makeconfig_arg3 = {0};
    /* ===== 调用 SignalComparatorThreshold_MakeConfig：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalComparatorThreshold_MakeConfig(signalcomparatorthreshold_makeconfig_arg0, signalcomparatorthreshold_makeconfig_arg1, signalcomparatorthreshold_makeconfig_arg2, &signalcomparatorthreshold_makeconfig_arg3);

    /* ===== 调用 SignalComparatorThreshold_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalComparatorThreshold_GetModuleStatus();

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

