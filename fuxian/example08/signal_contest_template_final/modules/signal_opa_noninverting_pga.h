#ifndef SIGNAL_OPA_NONINVERTING_PGA_H
#define SIGNAL_OPA_NONINVERTING_PGA_H

#include "signal_opa.h"
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param requested_gain `requested_gain`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param resistor_to_ground_ohm `resistor_to_ground_ohm`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param bias_voltage_v `bias_voltage_v`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param config 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。
 * @param feedback_resistor_ohm `feedback_resistor_ohm`（`float *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalOPANoninvertingPGA_MakeConfig(float requested_gain,
    float resistor_to_ground_ohm, float bias_voltage_v,
    signal_opa_config_t *config, float *feedback_resistor_ohm);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalOPANoninvertingPGA_GetModuleStatus(void);

#endif

