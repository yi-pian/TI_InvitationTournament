/* channel_delay_calibration 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_channel_delay_calibration.h"

void channel_delay_calibration_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalchanneldelaycalibration_apply_arg0 = 0.0f;
    static float signalchanneldelaycalibration_apply_arg1 = 0.0f;
    static signal_channel_delay_calibration_t signalchanneldelaycalibration_apply_arg2 = {0};
    static float signalchanneldelaycalibration_apply_arg3[16] = {0};
    /* ===== 调用 SignalChannelDelayCalibration_Apply：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalChannelDelayCalibration_Apply(signalchanneldelaycalibration_apply_arg0, signalchanneldelaycalibration_apply_arg1, &signalchanneldelaycalibration_apply_arg2, signalchanneldelaycalibration_apply_arg3);

    static float signalchanneldelaycalibration_compute_arg0 = 0.0f;
    static float signalchanneldelaycalibration_compute_arg1 = 0.0f;
    static float signalchanneldelaycalibration_compute_arg2 = 0.0f;
    static signal_channel_delay_calibration_t signalchanneldelaycalibration_compute_arg3 = {0};
    /* ===== 调用 SignalChannelDelayCalibration_Compute：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalChannelDelayCalibration_Compute(signalchanneldelaycalibration_compute_arg0, signalchanneldelaycalibration_compute_arg1, signalchanneldelaycalibration_compute_arg2, &signalchanneldelaycalibration_compute_arg3);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

