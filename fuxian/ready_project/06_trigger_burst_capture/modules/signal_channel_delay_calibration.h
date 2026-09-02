#ifndef SIGNAL_CHANNEL_DELAY_CALIBRATION_H
#define SIGNAL_CHANNEL_DELAY_CALIBRATION_H

#include "signal_algorithm_status.h"

typedef struct
{
    float delay_b_relative_to_a_s;
} signal_channel_delay_calibration_t;

/**
 * @brief 用已知同频相位真值估计 B 相对 A 的固定时间延迟。
 * @param measured_phase_b_minus_a_deg 实测 B-A 相位，deg。
 * @param expected_phase_b_minus_a_deg 参考真值，deg。
 * @param frequency_hz 校准频率，Hz。
 * @param calibration 输出 B 相对 A 延迟，正值表示 B 更晚，单位 s。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 * @note 相位误差先 wrap 到 [-180,180)，因此校准前真实延迟必须小于半周期或已知整周数。
 */
signal_algorithm_status_t SignalChannelDelayCalibration_Compute(
    float measured_phase_b_minus_a_deg,
    float expected_phase_b_minus_a_deg,
    float frequency_hz,
    signal_channel_delay_calibration_t *calibration);

/**
 * @brief 从实测 B-A 相位中补偿固定通道延迟。
 * @param measured_phase_b_minus_a_deg 实测相位，deg。
 * @param frequency_hz 当前频率，Hz。
 * @param calibration 固定 delay，s。
 * @param corrected_phase_b_minus_a_deg 输出补偿相位，范围 [-180,180)。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 */
signal_algorithm_status_t SignalChannelDelayCalibration_Apply(
    float measured_phase_b_minus_a_deg,
    float frequency_hz,
    const signal_channel_delay_calibration_t *calibration,
    float *corrected_phase_b_minus_a_deg);

#endif /* SIGNAL_CHANNEL_DELAY_CALIBRATION_H */
