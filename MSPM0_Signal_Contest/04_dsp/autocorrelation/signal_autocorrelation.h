#ifndef SIGNAL_AUTOCORRELATION_H
#define SIGNAL_AUTOCORRELATION_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    uint32_t lag_count;
} signal_autocorrelation_result_t;

typedef struct
{
    uint32_t period_lag_samples;
    float peak_coefficient;
    float frequency_hz;
} signal_autocorrelation_period_result_t;

/**
 * @brief 计算 lag=0~max_lag 的归一化自相关。
 * @param samples 输入数组，只读，通常先 RemoveDC。
 * @param count 样本点数。
 * @param max_lag_samples 最大 lag，必须小于 count。
 * @param coefficients 输出数组，容量至少 max_lag+1。
 * @param coefficient_capacity 输出容量。
 * @param result 输出 lag 数量。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 */
signal_algorithm_status_t SignalAutocorrelation_Process(
    const float *samples,
    uint32_t count,
    uint32_t max_lag_samples,
    float *coefficients,
    uint32_t coefficient_capacity,
    signal_autocorrelation_result_t *result);

/**
 * @brief 在自相关的非零 lag 区间找最高正峰并换算周期频率。
 * @param coefficients 自相关数组，索引即 lag。
 * @param coefficient_count 数组长度。
 * @param min_lag_samples 搜索最小 lag，必须至少 1。
 * @param max_lag_samples 搜索最大 lag。
 * @param sample_rate_hz 采样率，Hz。
 * @param result 输出整数周期 lag、峰系数和频率。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 * @note min_lag 过小会把接近 lag=0 的宽峰误认成周期。
 */
signal_algorithm_status_t SignalAutocorrelation_FindPeriod(
    const float *coefficients,
    uint32_t coefficient_count,
    uint32_t min_lag_samples,
    uint32_t max_lag_samples,
    float sample_rate_hz,
    signal_autocorrelation_period_result_t *result);

#endif /* SIGNAL_AUTOCORRELATION_H */
