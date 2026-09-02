#ifndef SIGNAL_FREQUENCY_SWEEP_H
#define SIGNAL_FREQUENCY_SWEEP_H

#include <stdbool.h>
#include <stddef.h>
#include "signal_status.h"

typedef struct {
    float start_hz;
    float stop_hz;
    size_t point_count;
    bool logarithmic;
} signal_frequency_sweep_config_t;
/**
 * @brief 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。
 * @param config 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。
 * @param frequencies_hz `frequencies_hz`（`float *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param capacity 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalFrequencySweep_Generate(
    const signal_frequency_sweep_config_t *config, float *frequencies_hz,
    size_t capacity);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalFrequencySweep_GetModuleStatus(void);

#endif

