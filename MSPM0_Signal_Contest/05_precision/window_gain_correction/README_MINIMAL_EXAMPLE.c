/* window_gain_correction 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_window_gain_correction.h"

void window_gain_correction_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalwindowgaincorrection_apply_arg0[16] = {0};
    static float signalwindowgaincorrection_apply_arg1[16] = {0};
    static uint32_t signalwindowgaincorrection_apply_arg2 = 0U;
    static uint32_t signalwindowgaincorrection_apply_arg3 = 0U;
    static float signalwindowgaincorrection_apply_arg4 = 0.0f;
    /* ===== 最小入口：SignalWindowGainCorrection_Apply ===== */
    (void)SignalWindowGainCorrection_Apply(signalwindowgaincorrection_apply_arg0, signalwindowgaincorrection_apply_arg1, signalwindowgaincorrection_apply_arg2, signalwindowgaincorrection_apply_arg3, signalwindowgaincorrection_apply_arg4);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

