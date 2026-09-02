#ifndef SIGNAL_SNR_H
#define SIGNAL_SNR_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    uint32_t start_bin;
    uint32_t end_bin;
} signal_bin_range_t;

typedef struct
{
    uint32_t signal_start_bin;
    uint32_t signal_end_bin;
    uint32_t analysis_start_bin;
    uint32_t analysis_end_bin;
    const signal_bin_range_t *excluded_ranges;
    uint32_t excluded_range_count;
} signal_snr_config_t;

typedef struct
{
    float signal_energy;
    float noise_energy;
    float snr_power_ratio;
    float snr_db;
    uint32_t noise_bin_count;
} signal_snr_result_t;

/**
 * @brief 在指定谱区间内，用目标 band 能量与其余未排除 bin 能量计算 SNR。
 * @param magnitude 非负线性 magnitude 数组。
 * @param bin_count 数组长度。
 * @param config 目标范围、分析范围和可选排除范围（如 DC/谐波）。
 * @param result 输出 signal/noise 能量、功率比、dB 和噪声 bin 数。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；无信号/噪声或范围非法返回错误。
 * @note 若不排除谐波，结果更接近 SINAD 而非严格 SNR。
 */
signal_algorithm_status_t SignalSNR_Process(
    const float *magnitude,
    uint32_t bin_count,
    const signal_snr_config_t *config,
    signal_snr_result_t *result);

#endif /* SIGNAL_SNR_H */
