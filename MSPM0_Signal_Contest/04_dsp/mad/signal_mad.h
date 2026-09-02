#ifndef SIGNAL_MAD_H
#define SIGNAL_MAD_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float median_value;
    float mad_value;
    float robust_sigma_estimate;
} signal_mad_result_t;

/**
 * @brief 计算中位数、绝对中位差 MAD 和高斯噪声等效 sigma 估计。
 * @param samples 输入样本，只读，单位任意但必须一致。
 * @param count 样本数，必须大于 0 且不超过 INT32_MAX。
 * @param workspace 调用者提供的临时 float 数组，会被重排覆盖。
 * @param workspace_count 临时数组容量，至少为 count。
 * @param result 输出中位数、MAD、1.4826*MAD，单位与输入相同。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数、空间或数值非法返回错误码。
 * @note 不修改原始 samples；不使用动态内存。
 */
signal_algorithm_status_t SignalMAD_Process(
    const float *samples,
    uint32_t count,
    float *workspace,
    uint32_t workspace_count,
    signal_mad_result_t *result);

#endif /* SIGNAL_MAD_H */
