#ifndef SIGNAL_JACOBSEN_INTERPOLATION_H
#define SIGNAL_JACOBSEN_INTERPOLATION_H

#include <stdint.h>

#include "signal_algorithm_status.h"
#include "signal_complex.h"

typedef struct
{
    float fractional_bin;
    float interpolated_bin;
    float frequency_hz;
} signal_jacobsen_result_t;
/**
 * @brief 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。
 * @param spectrum `spectrum`（`const signal_complex_f32_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param spectrum_count 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @param peak_index 索引或通道号；范围由相应数组长度、FFT bin 数或当前硬件配置决定。
 * @param sample_rate_hz 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。
 * @param fft_size 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @param result 由调用者分配的输出对象/数组。成功返回后才读取其中内容。
 * @return 返回 signal_algorithm_status_t 类型结果；调用者应检查该值。
 */

signal_algorithm_status_t SignalJacobsen_Process(
    const signal_complex_f32_t *spectrum,
    uint32_t spectrum_count,
    uint32_t peak_index,
    float sample_rate_hz,
    uint32_t fft_size,
    signal_jacobsen_result_t *result);

#endif

