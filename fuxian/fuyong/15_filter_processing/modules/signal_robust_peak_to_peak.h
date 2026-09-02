#ifndef SIGNAL_ROBUST_PEAK_TO_PEAK_H
#define SIGNAL_ROBUST_PEAK_TO_PEAK_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float lower_quantile;
    float upper_quantile;
} signal_robust_peak_to_peak_config_t;

typedef struct
{
    float lower_voltage_v;
    float upper_voltage_v;
    float robust_vpp_v;
} signal_robust_peak_to_peak_result_t;

/**
 * @brief 用上下分位数差代替 max-min，降低少量极端毛刺影响。
 * @param voltage_v 输入电压，V，只读。
 * @param count 点数，必须大于 0 且不超过 INT32_MAX。
 * @param config 0~1 内 lower<upper 的线性插值分位数。
 * @param workspace 调用者 float 工作区，容量至少 count，会被重排。
 * @param workspace_count 工作区容量。
 * @param result 输出上下分位电压与 robust Vpp，V。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 * @note 这不是物理最大峰峰值；分位数会主动忽略尾部样本。
 */
signal_algorithm_status_t SignalRobustPeakToPeak_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_robust_peak_to_peak_config_t *config,
    float *workspace,
    uint32_t workspace_count,
    signal_robust_peak_to_peak_result_t *result);

#endif /* SIGNAL_ROBUST_PEAK_TO_PEAK_H */
