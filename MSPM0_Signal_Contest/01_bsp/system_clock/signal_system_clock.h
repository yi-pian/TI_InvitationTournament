#ifndef SIGNAL_SYSTEM_CLOCK_H
#define SIGNAL_SYSTEM_CLOCK_H

#include <stdint.h>
#include "signal_status.h"

typedef struct {
    uint32_t cpu_hz;
    uint32_t bus_hz;
    uint32_t timer_hz;
    uint32_t adc_hz;
} signal_system_clock_config_t;
/**
 * @brief 检查配置或输入是否满足模块要求；建议在第一次接入或参数变化后调用。
 * @param config 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalSystemClock_Validate(const signal_system_clock_config_t *config);
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param timer_hz `timer_hz`（`uint32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param requested_rate_hz 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。
 * @param max_count 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @param timer_count 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @param configured_rate_hz 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */
signal_result_t SignalSystemClock_CalculateTimerPeriod(uint32_t timer_hz,
    uint32_t requested_rate_hz, uint32_t max_count, uint32_t *timer_count,
    uint32_t *configured_rate_hz);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalSystemClock_GetModuleStatus(void);

#endif

