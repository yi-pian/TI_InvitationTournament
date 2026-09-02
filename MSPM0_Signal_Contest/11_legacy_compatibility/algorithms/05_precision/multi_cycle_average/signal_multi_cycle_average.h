#ifndef SIGNAL_MULTI_CYCLE_AVERAGE_H
#define SIGNAL_MULTI_CYCLE_AVERAGE_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float frequency_hz;
    float average_period_samples;
    float observation_time_s;
    uint32_t cycle_count;
} signal_multi_cycle_average_result_t;

/**
 * @brief 用首末同方向过零位置跨越的多个周期计算平均周期和频率。
 * @param crossing_positions_samples 严格递增的同方向过零位置，单位 sample。
 * @param crossing_count 位置数量，至少为 2；周期数为 crossing_count-1。
 * @param sample_rate_hz 每通道采样率，单位 Hz，必须为正。
 * @param result 输出频率、平均周期、观测时间和周期数。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；数据不足、顺序或数值非法返回错误码。
 * @note 不要混入上升与下降过零，否则相邻位置只隔半周期，频率会翻倍。
 */
signal_algorithm_status_t SignalMultiCycleAverage_Process(
    const float *crossing_positions_samples,
    uint32_t crossing_count,
    float sample_rate_hz,
    signal_multi_cycle_average_result_t *result);

#endif /* SIGNAL_MULTI_CYCLE_AVERAGE_H */
