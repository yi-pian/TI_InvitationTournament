/* autocorrelation 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_autocorrelation.h"

void autocorrelation_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalautocorrelation_process_arg0[16] = {0};
    static uint32_t signalautocorrelation_process_arg1 = 0U;
    static uint32_t signalautocorrelation_process_arg2 = 0U;
    static float signalautocorrelation_process_arg3[16] = {0};
    static uint32_t signalautocorrelation_process_arg4 = 0U;
    static signal_autocorrelation_result_t signalautocorrelation_process_arg5 = {0};
    /* ===== 调用 SignalAutocorrelation_Process：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalAutocorrelation_Process(signalautocorrelation_process_arg0, signalautocorrelation_process_arg1, signalautocorrelation_process_arg2, signalautocorrelation_process_arg3, signalautocorrelation_process_arg4, &signalautocorrelation_process_arg5);

    static float signalautocorrelation_findperiod_arg0[16] = {0};
    static uint32_t signalautocorrelation_findperiod_arg1 = 0U;
    static uint32_t signalautocorrelation_findperiod_arg2 = 0U;
    static uint32_t signalautocorrelation_findperiod_arg3 = 0U;
    static float signalautocorrelation_findperiod_arg4 = 0.0f;
    static signal_autocorrelation_period_result_t signalautocorrelation_findperiod_arg5 = {0};
    /* ===== 调用 SignalAutocorrelation_FindPeriod：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalAutocorrelation_FindPeriod(signalautocorrelation_findperiod_arg0, signalautocorrelation_findperiod_arg1, signalautocorrelation_findperiod_arg2, signalautocorrelation_findperiod_arg3, signalautocorrelation_findperiod_arg4, &signalautocorrelation_findperiod_arg5);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

