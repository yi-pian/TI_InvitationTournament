#ifndef SIGNAL_LOCK_IN_H
#define SIGNAL_LOCK_IN_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float reference_frequency_hz;
    float sample_rate_hz;
    float reference_phase_rad;
    uint8_t remove_dc;
} signal_lock_in_config_t;

typedef struct
{
    float mean_voltage_v;
    float in_phase_v;
    float quadrature_v;
    float amplitude_peak_v;
    float phase_rad;
    float phase_deg;
} signal_lock_in_result_t;

/**
 * @brief 与已知余弦参考同步正交积分，提取目标频率的幅值和相位。
 * @param voltage_v 输入电压，V。
 * @param count 点数，至少 2；最好覆盖参考的整数周期。
 * @param config 参考频率/Fs/初相和是否去平均 DC。
 * @param result 输出均值、I/Q、峰值和相对参考相位。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 * @note 信号模型为 `A*cos(w n + reference_phase + phase)`；I=Acos、Q=Asin。
 */
signal_algorithm_status_t SignalLockIn_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_lock_in_config_t *config,
    signal_lock_in_result_t *result);

#endif /* SIGNAL_LOCK_IN_H */
