/* adc_gain_offset_calibration 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_adc_gain_offset_calibration.h"

void adc_gain_offset_calibration_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signaladcgainoffsetcalibration_apply_arg0[16] = {0};
    static float signaladcgainoffsetcalibration_apply_arg1[16] = {0};
    static uint32_t signaladcgainoffsetcalibration_apply_arg2 = 0U;
    static signal_adc_gain_offset_calibration_t signaladcgainoffsetcalibration_apply_arg3 = {0};
    /* ===== 调用 SignalADCGainOffsetCalibration_Apply：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADCGainOffsetCalibration_Apply(signaladcgainoffsetcalibration_apply_arg0, signaladcgainoffsetcalibration_apply_arg1, signaladcgainoffsetcalibration_apply_arg2, &signaladcgainoffsetcalibration_apply_arg3);

    static float signaladcgainoffsetcalibration_compute_arg0 = 0.0f;
    static float signaladcgainoffsetcalibration_compute_arg1 = 0.0f;
    static float signaladcgainoffsetcalibration_compute_arg2 = 0.0f;
    static float signaladcgainoffsetcalibration_compute_arg3 = 0.0f;
    static signal_adc_gain_offset_calibration_t signaladcgainoffsetcalibration_compute_arg4 = {0};
    /* ===== 调用 SignalADCGainOffsetCalibration_Compute：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADCGainOffsetCalibration_Compute(signaladcgainoffsetcalibration_compute_arg0, signaladcgainoffsetcalibration_compute_arg1, signaladcgainoffsetcalibration_compute_arg2, signaladcgainoffsetcalibration_compute_arg3, &signaladcgainoffsetcalibration_compute_arg4);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

