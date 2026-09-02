/* macleod_interpolation 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_macleod_interpolation.h"

void macleod_interpolation_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_complex_f32_t signalmacleod_process_arg0 = {0};
    static uint32_t signalmacleod_process_arg1 = 0U;
    static uint32_t signalmacleod_process_arg2 = 0U;
    static float signalmacleod_process_arg3 = 0.0f;
    static uint32_t signalmacleod_process_arg4 = 0U;
    static signal_macleod_result_t signalmacleod_process_arg5 = {0};
    /* ===== 调用 SignalMacleod_Process：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalMacleod_Process(&signalmacleod_process_arg0, signalmacleod_process_arg1, signalmacleod_process_arg2, signalmacleod_process_arg3, signalmacleod_process_arg4, &signalmacleod_process_arg5);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

