#ifndef SIGNAL_SFDR_H
#define SIGNAL_SFDR_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    uint32_t main_start_bin;
    uint32_t main_end_bin;
    uint32_t analysis_start_bin;
    uint32_t analysis_end_bin;
} signal_sfdr_config_t;

typedef struct
{
    uint32_t main_peak_bin;
    uint32_t spur_peak_bin;
    float main_peak_magnitude;
    float spur_peak_magnitude;
    float sfdr_ratio;
    float sfdr_db;
} signal_sfdr_result_t;

/**
 * @brief 计算主信号峰与分析范围内最大非主 band 杂散峰之比。
 * @param magnitude 非负线性 magnitude。
 * @param bin_count 数组长度。
 * @param config 主 band 和分析 band。
 * @param result 输出主峰/杂散索引、幅值、比值与 dB。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；范围或零峰返回错误。
 */
signal_algorithm_status_t SignalSFDR_Process(
    const float *magnitude,
    uint32_t bin_count,
    const signal_sfdr_config_t *config,
    signal_sfdr_result_t *result);

#endif /* SIGNAL_SFDR_H */
