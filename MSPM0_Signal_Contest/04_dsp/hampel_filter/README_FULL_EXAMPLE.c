/* hampel_filter 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_hampel.h"

void hampel_filter_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalhampel_process_arg0[16] = {0};
    static float signalhampel_process_arg1[16] = {0};
    static uint32_t signalhampel_process_arg2 = 0U;
    static signal_hampel_config_t signalhampel_process_arg3 = {0};
    static float signalhampel_process_arg4[16] = {0};
    static uint32_t signalhampel_process_arg5 = 0U;
    static signal_hampel_result_t signalhampel_process_arg6 = {0};
    /* ===== 调用 SignalHampel_Process：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalHampel_Process(signalhampel_process_arg0, signalhampel_process_arg1, signalhampel_process_arg2, &signalhampel_process_arg3, signalhampel_process_arg4, signalhampel_process_arg5, &signalhampel_process_arg6);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

