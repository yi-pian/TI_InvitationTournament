#ifndef SIGNAL_VPP_H
#define SIGNAL_VPP_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float amplitude_vpp;
    float min_voltage_v;
    float max_voltage_v;
} signal_vpp_result_t;

/**
 * @brief 用最大电压减最小电压计算峰峰值。
 * @param voltage_v 输入电压数组，单位 V，只读。
 * @param count 样本点数，必须大于 0。
 * @param result 输出峰峰值、最小电压和最大电压，单位均为 V。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法时返回错误码。
 * @note 必须覆盖信号的峰和谷；该方法对单点毛刺敏感。
 */
signal_algorithm_status_t SignalVPP_Process(
    const float *voltage_v,
    uint32_t count,
    signal_vpp_result_t *result);

#endif /* SIGNAL_VPP_H */
