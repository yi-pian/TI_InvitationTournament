#ifndef SIGNAL_ADC_DUAL_SYNC_H
#define SIGNAL_ADC_DUAL_SYNC_H

#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param interleaved `interleaved`（`const uint16_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param pair_count 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @param channel_a 索引或通道号；范围由相应数组长度、FFT bin 数或当前硬件配置决定。
 * @param channel_a_capacity 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @param channel_b 索引或通道号；范围由相应数组长度、FFT bin 数或当前硬件配置决定。
 * @param channel_b_capacity 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalADCDualSync_Deinterleave(const uint16_t *interleaved,
    size_t pair_count, uint16_t *channel_a, size_t channel_a_capacity,
    uint16_t *channel_b, size_t channel_b_capacity);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalADCDualSync_GetModuleStatus(void);

#endif

