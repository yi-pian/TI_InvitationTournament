#ifndef SIGNAL_ROBUST_RMS_H
#define SIGNAL_ROBUST_RMS_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float lower_quantile;
    float upper_quantile;
    uint8_t remove_dc;
} signal_robust_rms_config_t;

typedef struct
{
    float lower_limit_v;
    float upper_limit_v;
    float winsorized_mean_v;
    float robust_rms_v;
    uint32_t winsorized_count;
} signal_robust_rms_result_t;

/**
 * @brief 先按分位数把极端点钳到边界（Winsorize），再计算总/交流 RMS。
 * @param voltage_v 输入电压，V。
 * @param count 点数。
 * @param config 分位数和 remove_dc 标志。
 * @param workspace float 工作区，容量至少 count。
 * @param workspace_count 工作区容量。
 * @param result 输出边界、winsorized mean、RMS 和钳位点数。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 * @note 改变真实尾部；尖峰是目标时禁止使用。
 */
signal_algorithm_status_t SignalRobustRMS_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_robust_rms_config_t *config,
    float *workspace,
    uint32_t workspace_count,
    signal_robust_rms_result_t *result);

#endif /* SIGNAL_ROBUST_RMS_H */
