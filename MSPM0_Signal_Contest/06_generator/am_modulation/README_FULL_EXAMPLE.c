/* am_modulation 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_am_modulation.h"

void am_modulation_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalammodulation_apply_arg0[16] = {0};
    static float signalammodulation_apply_arg1[16] = {0};
    static size_t signalammodulation_apply_arg2 = 0U;
    static float signalammodulation_apply_arg3 = 0.0f;
    static float signalammodulation_apply_arg4[16] = {0};
    static size_t signalammodulation_apply_arg5 = 0U;
    /* ===== 调用 SignalAMModulation_Apply：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalAMModulation_Apply(signalammodulation_apply_arg0, signalammodulation_apply_arg1, signalammodulation_apply_arg2, signalammodulation_apply_arg3, signalammodulation_apply_arg4, signalammodulation_apply_arg5);

    /* ===== 调用 SignalAMModulation_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalAMModulation_GetModuleStatus();

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

