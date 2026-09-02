/* timer_capture 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_timer_capture.h"

void timer_capture_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    /* ===== 调用 SignalTimerCapture_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalTimerCapture_GetModuleStatus();

    static uint32_t signaltimercapture_delta_arg0 = 0U;
    static uint32_t signaltimercapture_delta_arg1 = 0U;
    static uint32_t signaltimercapture_delta_arg2 = 0U;
    static uint32_t signaltimercapture_delta_arg3[16] = {0};
    /* ===== 调用 SignalTimerCapture_Delta：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalTimerCapture_Delta(signaltimercapture_delta_arg0, signaltimercapture_delta_arg1, signaltimercapture_delta_arg2, signaltimercapture_delta_arg3);

    static uint32_t signaltimercapture_meanperiod_arg0[16] = {0};
    static size_t signaltimercapture_meanperiod_arg1 = 0U;
    static signal_timer_capture_config_t signaltimercapture_meanperiod_arg2 = {0};
    static float signaltimercapture_meanperiod_arg3[16] = {0};
    static float signaltimercapture_meanperiod_arg4[16] = {0};
    /* ===== 调用 SignalTimerCapture_MeanPeriod：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalTimerCapture_MeanPeriod(signaltimercapture_meanperiod_arg0, signaltimercapture_meanperiod_arg1, &signaltimercapture_meanperiod_arg2, signaltimercapture_meanperiod_arg3, signaltimercapture_meanperiod_arg4);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

