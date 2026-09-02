#ifndef SIGNAL_ZERO_CROSS_H
#define SIGNAL_ZERO_CROSS_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef enum
{
    SIGNAL_ZERO_CROSS_RISING = 0,
    SIGNAL_ZERO_CROSS_FALLING,
    SIGNAL_ZERO_CROSS_BOTH
} signal_zero_cross_direction_t;

typedef struct
{
    float threshold_v;
    float hysteresis_v;
    signal_zero_cross_direction_t direction;
} signal_zero_cross_config_t;

typedef struct
{
    uint32_t left_index;
    uint32_t right_index;
    signal_zero_cross_direction_t direction;
} signal_zero_cross_event_t;

typedef struct
{
    uint32_t event_count;
    uint32_t rising_count;
    uint32_t falling_count;
} signal_zero_cross_result_t;

/**
 * @brief 在电压样本中查找跨越指定阈值的相邻样本对。
 * @param voltage_v 输入电压数组，单位 V，只读。
 * @param count 样本点数，至少为 2。
 * @param config 阈值、非负滞回和方向配置，单位 V。
 * @param events 调用者提供的事件数组；每个事件保存阈值左右的相邻索引。
 * @param event_capacity events 可容纳的元素数，必须大于 0。
 * @param result 输出事件数量及上升/下降数量。
 * @return 找到事件返回 SIGNAL_ALGORITHM_OK；未找到、空间不足或参数非法返回对应状态。
 * @note 只定位整数样本夹点；亚采样位置由 SignalZeroCrossInterpolation_Process 计算。
 */
signal_algorithm_status_t SignalZeroCross_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_zero_cross_config_t *config,
    signal_zero_cross_event_t *events,
    uint32_t event_capacity,
    signal_zero_cross_result_t *result);

#endif /* SIGNAL_ZERO_CROSS_H */
