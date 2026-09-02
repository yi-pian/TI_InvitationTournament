#ifndef SIGNAL_WINDOW_H
#define SIGNAL_WINDOW_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef enum
{
    SIGNAL_WINDOW_RECTANGULAR = 0,
    SIGNAL_WINDOW_HANN,
    SIGNAL_WINDOW_HAMMING,
    SIGNAL_WINDOW_BLACKMAN
} signal_window_type_t;

typedef struct
{
    float coherent_gain;
    float power_gain;
} signal_window_result_t;

/**
 * @brief 生成并逐点应用指定窗，同时返回相干增益和功率增益。
 * @param input_samples 输入数组，只读，单位任意。
 * @param output_samples 输出数组，容量至少 count；允许与输入为同一数组。
 * @param count 点数，至少为 2。
 * @param type Rectangular/Hann/Hamming/Blackman。
 * @param result 输出 `mean(w)` 相干增益和 `mean(w^2)` 功率增益。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法返回错误码。
 * @note 使用对称窗定义，分母为 count-1；不保存窗系数数组，节省 RAM。
 */
signal_algorithm_status_t SignalWindow_Apply(
    const float *input_samples,
    float *output_samples,
    uint32_t count,
    signal_window_type_t type,
    signal_window_result_t *result);

#endif /* SIGNAL_WINDOW_H */
