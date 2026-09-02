#ifndef SIGNAL_ZERO_CROSS_INTERPOLATION_H
#define SIGNAL_ZERO_CROSS_INTERPOLATION_H

#include <stdint.h>

#include "signal_algorithm_status.h"
#include "signal_zero_cross.h"

typedef struct
{
    uint32_t position_count;
} signal_zero_cross_interpolation_result_t;

/**
 * @brief 对过零事件两侧样本做直线插值，得到带小数的样本位置。
 * @param voltage_v 原始或去直流电压数组，单位 V，只读。
 * @param sample_count voltage_v 的总点数。
 * @param threshold_v 过零检测使用的同一阈值，单位 V。
 * @param events SignalZeroCross_Process 输出的事件数组。
 * @param event_count 事件数，必须大于 0。
 * @param crossing_positions_samples 输出位置，单位 sample，例如 12.25 表示第 12 与 13 点之间。
 * @param position_capacity 输出容量，至少为 event_count。
 * @param result 输出有效位置数量。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；索引、夹点、空间或数值非法返回错误码。
 * @note 假设阈值附近两点之间近似直线；不会改变输入数组。
 */
signal_algorithm_status_t SignalZeroCrossInterpolation_Process(
    const float *voltage_v,
    uint32_t sample_count,
    float threshold_v,
    const signal_zero_cross_event_t *events,
    uint32_t event_count,
    float *crossing_positions_samples,
    uint32_t position_capacity,
    signal_zero_cross_interpolation_result_t *result);

#endif /* SIGNAL_ZERO_CROSS_INTERPOLATION_H */
