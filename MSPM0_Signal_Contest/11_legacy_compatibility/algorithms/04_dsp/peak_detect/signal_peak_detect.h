#ifndef SIGNAL_PEAK_DETECT_H
#define SIGNAL_PEAK_DETECT_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    uint32_t peak_index;
    float peak_value;
} signal_peak_detect_result_t;

/**
 * @brief 在闭区间 [start_index,end_index] 中寻找首次出现的最大谱值。
 * @param values 输入非负幅值/能量数组，只读。
 * @param count 数组总元素数。
 * @param start_index 搜索起点，常设 1 以排除 DC。
 * @param end_index 搜索终点，包含在内。
 * @param result 输出峰索引和峰值，单位与输入相同。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；范围或数值非法返回错误码。
 */
signal_algorithm_status_t SignalPeakDetect_Process(
    const float *values,
    uint32_t count,
    uint32_t start_index,
    uint32_t end_index,
    signal_peak_detect_result_t *result);

#endif /* SIGNAL_PEAK_DETECT_H */
