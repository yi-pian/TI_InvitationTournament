#ifndef SIGNAL_PHASE_H
#define SIGNAL_PHASE_H

#include <stdint.h>

#include "signal_algorithm_status.h"
#include "signal_complex.h"

typedef struct
{
    float phase_difference_deg;
    float phase_difference_rad;
} signal_phase_result_t;

/**
 * @brief 用两路同方向过零位置与周期计算 B-A 相位差。
 * @param crossing_a_samples A 路过零位置，sample。
 * @param crossing_b_samples B 路对应过零位置，sample。
 * @param period_samples 周期，sample，必须为正。
 * @param result 输出 B-A 相位，范围 [-180,180)，单位 deg/rad。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 * @note B 过零更晚表示 B 滞后，因此 B-A 为负相位。
 */
signal_algorithm_status_t SignalPhase_FromZeroCross(
    float crossing_a_samples,
    float crossing_b_samples,
    float period_samples,
    signal_phase_result_t *result);

/**
 * @brief 用同一 FFT bin 的两路复数相角计算 B-A 相位差。
 * @param spectrum_a A 路 N 点复频谱。
 * @param spectrum_b B 路 N 点复频谱。
 * @param spectrum_count 两数组长度。
 * @param bin_index 目标频率 bin。
 * @param result 输出 B-A 相位 deg/rad。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；目标 bin 幅值为零返回 NO_FEATURE。
 */
signal_algorithm_status_t SignalPhase_FromFFTBin(
    const signal_complex_f32_t *spectrum_a,
    const signal_complex_f32_t *spectrum_b,
    uint32_t spectrum_count,
    uint32_t bin_index,
    signal_phase_result_t *result);

/**
 * @brief 把互相关峰值 lag 换算为 B-A 相位差。
 * @param lag_b_relative_to_a_samples 正值表示 B 比 A 晚，单位 sample。
 * @param period_samples 周期，sample。
 * @param result 输出 B-A 相位。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 */
signal_algorithm_status_t SignalPhase_FromCorrelationLag(
    float lag_b_relative_to_a_samples,
    float period_samples,
    signal_phase_result_t *result);

#endif /* SIGNAL_PHASE_H */
