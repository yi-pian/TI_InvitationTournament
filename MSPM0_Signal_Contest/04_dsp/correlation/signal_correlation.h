#ifndef SIGNAL_CORRELATION_H
#define SIGNAL_CORRELATION_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    int32_t best_lag_samples;
    float best_coefficient;
    int32_t best_absolute_lag_samples;
    float best_absolute_coefficient;
} signal_correlation_result_t;

/**
 * @brief 计算归一化互相关 R_ab[lag]=corr(a[n],b[n+lag])。
 * @param samples_a A 路输入，只读。
 * @param samples_b B 路输入，只读，与 A 等长。
 * @param count 每路点数。
 * @param max_lag_samples 搜索 -max_lag~+max_lag，必须小于 count。
 * @param coefficients 输出长度 2*max_lag+1；索引 lag+max_lag。
 * @param coefficient_capacity 输出容量。
 * @param result 输出最大正相关和最大绝对相关的 lag/系数。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；能量为零或参数非法返回错误。
 * @note 正 lag 表示 B 相对 A 更晚；通常先分别 RemoveDC。
 */
signal_algorithm_status_t SignalCorrelation_Process(
    const float *samples_a,
    const float *samples_b,
    uint32_t count,
    uint32_t max_lag_samples,
    float *coefficients,
    uint32_t coefficient_capacity,
    signal_correlation_result_t *result);

#endif /* SIGNAL_CORRELATION_H */
