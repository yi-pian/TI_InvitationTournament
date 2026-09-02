#ifndef SIGNAL_TIMER_CAPTURE_H
#define SIGNAL_TIMER_CAPTURE_H

#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

typedef struct {
    uint32_t timer_hz;
    uint32_t counter_modulus;
} signal_timer_capture_config_t;
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param earlier `earlier`（`uint32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param later `later`（`uint32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param counter_modulus 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @param delta_ticks `delta_ticks`（`uint32_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalTimerCapture_Delta(uint32_t earlier, uint32_t later,
    uint32_t counter_modulus, uint32_t *delta_ticks);
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param timestamps `timestamps`（`const uint32_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param timestamp_count 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @param config 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。
 * @param mean_ticks `mean_ticks`（`float *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param frequency_hz 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */
signal_result_t SignalTimerCapture_MeanPeriod(const uint32_t *timestamps,
    size_t timestamp_count, const signal_timer_capture_config_t *config,
    float *mean_ticks, float *frequency_hz);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalTimerCapture_GetModuleStatus(void);

#endif

