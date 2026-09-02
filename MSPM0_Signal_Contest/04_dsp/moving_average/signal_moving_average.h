#ifndef SIGNAL_MOVING_AVERAGE_H
#define SIGNAL_MOVING_AVERAGE_H

#include <stdint.h>

#include "signal_algorithm_status.h"

/**
 * @brief 对每个样本计算“当前点及其之前若干点”的因果滑动平均。
 * @param input_samples 输入数组，只读，单位由调用者决定。
 * @param output_samples 输出数组，单位与输入相同，容量至少为 count；不得与输入重叠。
 * @param count 样本点数，必须大于 0。
 * @param window_size 平均窗口点数，范围 1~count；帧起始处使用已有的较短窗口。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法返回错误码。
 * @note 块首没有继承上一帧历史，因此流式连续滤波需由应用层保留重叠数据或使用后续流式版本。
 */
signal_algorithm_status_t SignalMovingAverage_Process(
    const float *input_samples,
    float *output_samples,
    uint32_t count,
    uint32_t window_size);

#endif /* SIGNAL_MOVING_AVERAGE_H */
