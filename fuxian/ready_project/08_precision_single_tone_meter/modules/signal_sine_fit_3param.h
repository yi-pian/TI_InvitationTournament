#ifndef SIGNAL_SINE_FIT_3PARAM_H
#define SIGNAL_SINE_FIT_3PARAM_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float frequency_hz;
    float sample_rate_hz;
} signal_sine_fit_3param_config_t;

typedef struct
{
    float cosine_coefficient_v;
    float sine_coefficient_v;
    float dc_offset_v;
    float amplitude_peak_v;
    float phase_rad;
    float phase_deg;
    float residual_rms_v;
} signal_sine_fit_3param_result_t;

/**
 * @brief 在已知频率下最小二乘拟合 `x=C*cos(w n)+S*sin(w n)+DC`。
 * @param voltage_v 输入电压，V。
 * @param count 点数，至少 3。
 * @param config 已知频率和采样率，Hz。
 * @param result 输出系数、DC、峰值、cos 模型相位和残差 RMS。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；矩阵奇异或参数非法返回错误。
 * @note phase 对应 `A*cos(w n + phase)`，因此 phase=atan2(-S,C)。
 */
signal_algorithm_status_t SignalSineFit3Param_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_sine_fit_3param_config_t *config,
    signal_sine_fit_3param_result_t *result);

#endif /* SIGNAL_SINE_FIT_3PARAM_H */
