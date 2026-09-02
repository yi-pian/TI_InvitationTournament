/* window 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_window.h"

void window_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalwindow_apply_arg0[16] = {0};
    static float signalwindow_apply_arg1[16] = {0};
    static uint32_t signalwindow_apply_arg2 = 0U;
    static signal_window_type_t signalwindow_apply_arg3 = 0U;
    static signal_window_result_t signalwindow_apply_arg4 = {0};
    /* ===== 调用 SignalWindow_Apply：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalWindow_Apply(signalwindow_apply_arg0, signalwindow_apply_arg1, signalwindow_apply_arg2, signalwindow_apply_arg3, &signalwindow_apply_arg4);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

