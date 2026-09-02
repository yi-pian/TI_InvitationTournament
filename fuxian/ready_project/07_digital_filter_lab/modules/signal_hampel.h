#ifndef SIGNAL_HAMPEL_H
#define SIGNAL_HAMPEL_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    uint32_t window_size;
    float threshold_sigma;
    float minimum_scale;
} signal_hampel_config_t;

typedef struct
{
    uint32_t replaced_count;
} signal_hampel_result_t;

/**
 * @brief 用局部中位数和 MAD 检测孤立离群点，并以局部中位数替换。
 * @param input_samples 输入数组，只读；不得与 output_samples 重叠。
 * @param output_samples 输出数组，容量至少 count，单位与输入相同。
 * @param count 样本点数。
 * @param config 奇数窗口、sigma 阈值、与输入同单位的最小尺度。
 * @param workspace 临时 float 数组，会被重排覆盖。
 * @param workspace_count 临时容量，至少为 window_size。
 * @param result 输出被替换点数。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数、空间或数值非法返回错误码。
 * @note 会删除被判为异常的尖峰；真实脉冲和瞬态测量禁止盲用。
 */
signal_algorithm_status_t SignalHampel_Process(
    const float *input_samples,
    float *output_samples,
    uint32_t count,
    const signal_hampel_config_t *config,
    float *workspace,
    uint32_t workspace_count,
    signal_hampel_result_t *result);

#endif /* SIGNAL_HAMPEL_H */
