/* sine_fit_3param 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_sine_fit_3param.h"

void sine_fit_3param_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalsinefit3param_process_arg0[16] = {0};
    static uint32_t signalsinefit3param_process_arg1 = 0U;
    static signal_sine_fit_3param_config_t signalsinefit3param_process_arg2 = {0};
    static signal_sine_fit_3param_result_t signalsinefit3param_process_arg3 = {0};
    /* ===== 最小入口：SignalSineFit3Param_Process ===== */
    (void)SignalSineFit3Param_Process(signalsinefit3param_process_arg0, signalsinefit3param_process_arg1, &signalsinefit3param_process_arg2, &signalsinefit3param_process_arg3);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

