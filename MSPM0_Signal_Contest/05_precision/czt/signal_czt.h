#ifndef SIGNAL_CZT_H
#define SIGNAL_CZT_H

#include <stdint.h>

#include "signal_algorithm_status.h"
#include "signal_complex.h"
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param samples 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。
 * @param sample_count 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @param sample_rate_hz 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。
 * @param start_frequency_hz 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。
 * @param frequency_step_hz `frequency_step_hz`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param output 由调用者分配的输出对象/数组。成功返回后才读取其中内容。
 * @param output_count 由调用者分配的输出对象/数组。成功返回后才读取其中内容。
 * @param output_capacity 由调用者分配的输出对象/数组。成功返回后才读取其中内容。
 * @return 返回 signal_algorithm_status_t 类型结果；调用者应检查该值。
 */

signal_algorithm_status_t SignalCZT_UnitCircleRealDirect(
    const float *samples,
    uint32_t sample_count,
    float sample_rate_hz,
    float start_frequency_hz,
    float frequency_step_hz,
    signal_complex_f32_t *output,
    uint32_t output_count,
    uint32_t output_capacity);

#endif

