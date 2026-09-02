#ifndef SIGNAL_AM_MODULATION_H
#define SIGNAL_AM_MODULATION_H

#include <stddef.h>
#include "signal_status.h"
/**
 * @brief 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。
 * @param carrier `carrier`（`const float *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param message `message`（`const float *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param count 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @param modulation_index 索引或通道号；范围由相应数组长度、FFT bin 数或当前硬件配置决定。
 * @param output 由调用者分配的输出对象/数组。成功返回后才读取其中内容。
 * @param output_capacity 由调用者分配的输出对象/数组。成功返回后才读取其中内容。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalAMModulation_Apply(const float *carrier,
    const float *message, size_t count, float modulation_index,
    float *output, size_t output_capacity);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalAMModulation_GetModuleStatus(void);

#endif

