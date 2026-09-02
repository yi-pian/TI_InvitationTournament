#ifndef SIGNAL_VREF_H
#define SIGNAL_VREF_H

#include <stdint.h>
#include "signal_status.h"

typedef struct {
    float nominal_voltage_v;
    float measured_voltage_v;
} signal_vref_calibration_t;
/**
 * @brief 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。
 * @param calibration `calibration`（`const signal_vref_calibration_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param voltage_v `voltage_v`（`float *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalVREF_GetEffectiveVoltage(
    const signal_vref_calibration_t *calibration, float *voltage_v);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalVREF_GetModuleStatus(void);

#endif

