/* am_modulation 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_am_modulation.h"

void am_modulation_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalammodulation_apply_arg0[16] = {0};
    static float signalammodulation_apply_arg1[16] = {0};
    static size_t signalammodulation_apply_arg2 = 0U;
    static float signalammodulation_apply_arg3 = 0.0f;
    static float signalammodulation_apply_arg4[16] = {0};
    static size_t signalammodulation_apply_arg5 = 0U;
    /* ===== 最小入口：SignalAMModulation_Apply ===== */
    (void)SignalAMModulation_Apply(signalammodulation_apply_arg0, signalammodulation_apply_arg1, signalammodulation_apply_arg2, signalammodulation_apply_arg3, signalammodulation_apply_arg4, signalammodulation_apply_arg5);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

