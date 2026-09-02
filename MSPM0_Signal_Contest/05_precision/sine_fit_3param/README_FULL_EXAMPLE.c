/* sine_fit_3param 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_sine_fit_3param.h"

void sine_fit_3param_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalsinefit3param_process_arg0[16] = {0};
    static uint32_t signalsinefit3param_process_arg1 = 0U;
    static signal_sine_fit_3param_config_t signalsinefit3param_process_arg2 = {0};
    static signal_sine_fit_3param_result_t signalsinefit3param_process_arg3 = {0};
    /* ===== 调用 SignalSineFit3Param_Process：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalSineFit3Param_Process(signalsinefit3param_process_arg0, signalsinefit3param_process_arg1, &signalsinefit3param_process_arg2, &signalsinefit3param_process_arg3);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

