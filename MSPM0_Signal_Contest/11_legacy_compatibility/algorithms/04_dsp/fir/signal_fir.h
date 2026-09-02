#ifndef SIGNAL_FIR_H
#define SIGNAL_FIR_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    const float *coefficients;
    float *delay_line;
    uint32_t tap_count;
    uint32_t write_index;
    uint8_t initialized;
} signal_fir_t;

/**
 * @brief 初始化外部系数 FIR；清零调用者提供的延迟线。
 * @param instance FIR 实例。
 * @param coefficients 系数数组，`coefficients[0]` 乘当前样本；只读且生命周期覆盖使用期。
 * @param tap_count 系数个数，必须大于 0。
 * @param delay_line 调用者提供的状态数组，容量至少 tap_count 个 float。
 * @param delay_line_count 状态数组容量。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 */
signal_algorithm_status_t SignalFIR_Init(
    signal_fir_t *instance,
    const float *coefficients,
    uint32_t tap_count,
    float *delay_line,
    uint32_t delay_line_count);

/** @brief 清零 FIR 历史状态，不改变系数；instance 必须已初始化。 */
signal_algorithm_status_t SignalFIR_Reset(signal_fir_t *instance);

/**
 * @brief 对一块浮点样本执行 FIR 卷积。
 * @param instance 已初始化 FIR 实例。
 * @param input_samples 输入数组，单位任意。
 * @param output_samples 输出数组，容量至少 count；允许与输入为同一数组。
 * @param count 样本点数，必须大于 0。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；状态或数值非法返回错误码。
 * @note 状态跨块保留；系数决定截止频率、增益和相位，模块不内置固定截止频率。
 */
signal_algorithm_status_t SignalFIR_Process(
    signal_fir_t *instance,
    const float *input_samples,
    float *output_samples,
    uint32_t count);

#endif /* SIGNAL_FIR_H */
