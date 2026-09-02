#ifndef SIGNAL_MEDIAN_FILTER_H
#define SIGNAL_MEDIAN_FILTER_H

#include <stdint.h>

#include "signal_algorithm_status.h"

/**
 * @brief 用每点附近窗口的中位数替换该点，抑制孤立尖峰。
 * @param input_samples 输入数组，只读；不得与 output_samples 重叠。
 * @param output_samples 输出数组，单位与输入相同，容量至少 count。
 * @param count 样本点数，必须大于 0。
 * @param window_size 奇数窗口长度，范围 1~count；边缘自动缩短窗口。
 * @param workspace 调用者提供的临时 float 数组，会被排序覆盖。
 * @param workspace_count 临时数组容量，至少为 window_size。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数、容量或数值非法返回错误码。
 * @note 非线性滤波器，可能改变频谱和真实窄脉冲。
 */
signal_algorithm_status_t SignalMedianFilter_Process(
    const float *input_samples,
    float *output_samples,
    uint32_t count,
    uint32_t window_size,
    float *workspace,
    uint32_t workspace_count);

#endif /* SIGNAL_MEDIAN_FILTER_H */
