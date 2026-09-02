/* adc_gain_offset_calibration 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_adc_gain_offset_calibration.h"

void adc_gain_offset_calibration_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signaladcgainoffsetcalibration_apply_arg0[16] = {0};
    static float signaladcgainoffsetcalibration_apply_arg1[16] = {0};
    static uint32_t signaladcgainoffsetcalibration_apply_arg2 = 0U;
    static signal_adc_gain_offset_calibration_t signaladcgainoffsetcalibration_apply_arg3 = {0};
    /* ===== 最小入口：SignalADCGainOffsetCalibration_Apply ===== */
    (void)SignalADCGainOffsetCalibration_Apply(signaladcgainoffsetcalibration_apply_arg0, signaladcgainoffsetcalibration_apply_arg1, signaladcgainoffsetcalibration_apply_arg2, &signaladcgainoffsetcalibration_apply_arg3);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

