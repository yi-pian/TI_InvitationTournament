#ifndef SIGNAL_SINE_FIT_4PARAM_H
#define SIGNAL_SINE_FIT_4PARAM_H

#include <stdint.h>

#include "signal_algorithm_status.h"
#include "signal_sine_fit_3param.h"

typedef struct
{
    float initial_frequency_hz;
    float search_half_width_hz;
    float sample_rate_hz;
    uint32_t iteration_count;
} signal_sine_fit_4param_config_t;

typedef struct
{
    float frequency_hz;
    uint32_t iteration_count;
    signal_sine_fit_3param_result_t waveform;
} signal_sine_fit_4param_result_t;

/**
 * @brief 在用户给定窄频带内用黄金分割搜索频率，每个候选做 3 参数最小二乘。
 * @param voltage_v 输入电压，V。
 * @param count 点数，至少 4。
 * @param config 初值、搜索半宽、Fs 和 6~40 次迭代。
 * @param result 输出频率及对应幅值/相位/DC/残差。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 * @note 只保证窄范围单峰目标；搜索范围跨多个谱主瓣时可能落入局部极小。
 */
signal_algorithm_status_t SignalSineFit4Param_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_sine_fit_4param_config_t *config,
    signal_sine_fit_4param_result_t *result);

#endif /* SIGNAL_SINE_FIT_4PARAM_H */
