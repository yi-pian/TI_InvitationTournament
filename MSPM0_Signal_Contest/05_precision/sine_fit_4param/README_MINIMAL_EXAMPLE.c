/* sine_fit_4param 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_sine_fit_4param.h"

void sine_fit_4param_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalsinefit4param_process_arg0[16] = {0};
    static uint32_t signalsinefit4param_process_arg1 = 0U;
    static signal_sine_fit_4param_config_t signalsinefit4param_process_arg2 = {0};
    static signal_sine_fit_4param_result_t signalsinefit4param_process_arg3 = {0};
    /* ===== 最小入口：SignalSineFit4Param_Process ===== */
    (void)SignalSineFit4Param_Process(signalsinefit4param_process_arg0, signalsinefit4param_process_arg1, &signalsinefit4param_process_arg2, &signalsinefit4param_process_arg3);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

