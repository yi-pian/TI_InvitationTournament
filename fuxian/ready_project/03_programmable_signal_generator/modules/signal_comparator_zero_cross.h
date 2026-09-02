#ifndef SIGNAL_COMPARATOR_ZERO_CROSS_H
#define SIGNAL_COMPARATOR_ZERO_CROSS_H

#include "signal_comparator.h"
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param virtual_ground_v `virtual_ground_v`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param hysteresis_v `hysteresis_v`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param config 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalComparatorZeroCross_MakeConfig(float virtual_ground_v,
    float hysteresis_v, signal_comparator_config_t *config);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalComparatorZeroCross_GetModuleStatus(void);

#endif

