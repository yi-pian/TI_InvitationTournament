#ifndef SIGNAL_REMOVE_DC_H
#define SIGNAL_REMOVE_DC_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float removed_mean_v;
} signal_remove_dc_result_t;

/**
 * @brief 从每个电压样本中减去整段样本的平均值。
 * @param input_voltage_v 输入电压数组，单位 V，只读；允许与 output_centered_v 指向同一数组。
 * @param output_centered_v 输出零偏信号，单位 V，容量至少为 count 个 float。
 * @param count 样本点数，必须大于 0。
 * @param result 输出被减掉的平均电压，单位 V。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法时返回错误码。
 * @note 支持原地处理；这会覆盖原始电压。只消除常量均值，不消除缓慢漂移。
 */
signal_algorithm_status_t SignalRemoveDC_Process(
    const float *input_voltage_v,
    float *output_centered_v,
    uint32_t count,
    signal_remove_dc_result_t *result);

#endif /* SIGNAL_REMOVE_DC_H */
