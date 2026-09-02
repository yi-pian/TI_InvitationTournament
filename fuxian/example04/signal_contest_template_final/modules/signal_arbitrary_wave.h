#ifndef SIGNAL_ARBITRARY_WAVE_H
#define SIGNAL_ARBITRARY_WAVE_H

#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param source `source`（`const uint16_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param source_count 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @param destination `destination`（`uint16_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param destination_count 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalArbitraryWave_ResampleLinear(const uint16_t *source,
    size_t source_count, uint16_t *destination, size_t destination_count);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalArbitraryWave_GetModuleStatus(void);

#endif

