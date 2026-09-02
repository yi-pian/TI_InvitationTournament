#ifndef SIGNAL_ADC_GAIN_OFFSET_CALIBRATION_H
#define SIGNAL_ADC_GAIN_OFFSET_CALIBRATION_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float gain;
    float offset_v;
} signal_adc_gain_offset_calibration_t;

/**
 * @brief 从两个已知真值点计算 `corrected = gain*measured + offset`。
 * @param measured_low_v 低点测量值，V。
 * @param true_low_v 低点参考真值，V。
 * @param measured_high_v 高点测量值，V，必须与低点不同。
 * @param true_high_v 高点参考真值，V。
 * @param calibration 输出无量纲 gain 和 V offset。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 */
signal_algorithm_status_t SignalADCGainOffsetCalibration_Compute(
    float measured_low_v,
    float true_low_v,
    float measured_high_v,
    float true_high_v,
    signal_adc_gain_offset_calibration_t *calibration);

/**
 * @brief 对电压数组应用增益/零偏校准，允许原地处理。
 * @param input_voltage_v 输入测量电压，V。
 * @param output_voltage_v 输出校准电压，V。
 * @param count 点数。
 * @param calibration 已计算校准参数。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 */
signal_algorithm_status_t SignalADCGainOffsetCalibration_Apply(
    const float *input_voltage_v,
    float *output_voltage_v,
    uint32_t count,
    const signal_adc_gain_offset_calibration_t *calibration);

#endif /* SIGNAL_ADC_GAIN_OFFSET_CALIBRATION_H */
