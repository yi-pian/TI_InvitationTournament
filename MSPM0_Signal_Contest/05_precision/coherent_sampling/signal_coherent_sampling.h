#ifndef SIGNAL_COHERENT_SAMPLING_H
#define SIGNAL_COHERENT_SAMPLING_H

#include <stdbool.h>
#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    uint32_t cycles_per_record;
    uint32_t samples_per_record;
    uint32_t cycle_sample_gcd;
    float coherent_frequency_hz;
    float frequency_error_hz;
    float absolute_error_hz;
    float relative_error_ppm;
} signal_coherent_sampling_result_t;
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param a `a`（`uint32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param b `b`（`uint32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 uint32_t 类型结果；调用者应检查该值。
 */

uint32_t SignalCoherentSampling_GCDU32(uint32_t a, uint32_t b);
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param desired_frequency_hz 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。
 * @param sample_rate_hz 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。
 * @param sample_count 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @param minimum_cycles `minimum_cycles`（`uint32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param maximum_cycles `maximum_cycles`（`uint32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param require_coprime `require_coprime`（`bool`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param result 由调用者分配的输出对象/数组。成功返回后才读取其中内容。
 * @return 返回 signal_algorithm_status_t 类型结果；调用者应检查该值。
 */

signal_algorithm_status_t SignalCoherentSampling_FindNearest(
    float desired_frequency_hz,
    float sample_rate_hz,
    uint32_t sample_count,
    uint32_t minimum_cycles,
    uint32_t maximum_cycles,
    bool require_coprime,
    signal_coherent_sampling_result_t *result);

#endif

