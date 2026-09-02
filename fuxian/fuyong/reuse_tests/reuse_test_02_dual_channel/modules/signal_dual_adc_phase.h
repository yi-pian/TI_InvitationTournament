#ifndef SIGNAL_DUAL_ADC_PHASE_H
#define SIGNAL_DUAL_ADC_PHASE_H

#include <stdint.h>

#include "signal_algorithm_status.h"

/* 固定上限避免模块内部动态分配；按本题 1.5~2 kHz、500 kSPS 设计。 */
#define SIGNAL_DUAL_ADC_PHASE_MAX_X_CROSSINGS (32U)
#define SIGNAL_DUAL_ADC_PHASE_MAX_Y_CROSSINGS (128U)

typedef struct
{
    uint16_t hysteresis_code;
    uint16_t min_amplitude_code;
    uint8_t frequency_ratio;
    uint16_t max_x_crossings;
    uint16_t max_y_crossings;
} signal_dual_adc_phase_config_t;

typedef struct
{
    int16_t phase_degrees;
    uint16_t valid_phase_count;
    uint16_t x_crossing_count;
    uint16_t y_crossing_count;
    uint8_t valid;
} signal_dual_adc_phase_result_t;

/**
 * @brief 从一帧双路同步 ADC 原始码计算 Y 相对 X 的相位差。
 * @param samples_x X 路 ADC 原始码数组，只读。
 * @param samples_y Y 路 ADC 原始码数组，只读，与 X 路同步且等长。
 * @param sample_count 每路数组的样本数，至少 2，不能超过 65535。
 * @param sample_rate_hz 实际同步采样率，单位 Hz，必须大于 0。
 * @param config 检测参数；frequency_ratio 为 fY/fX，范围 1~5。
 * @param result 输出结果；phase_degrees 范围 [-180,180]，正值表示 Y 超前。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数、数据量、幅度或数值不满足条件时返回对应错误码。
 * @note 模块不修改输入数组，不需要 SysConfig，不使用动态内存。采样率用于校验接口；相位比值中 Fs 会约掉。
 */
signal_algorithm_status_t SignalDualADCPhase_Process(
    const uint16_t *samples_x,
    const uint16_t *samples_y,
    uint32_t sample_count,
    uint32_t sample_rate_hz,
    const signal_dual_adc_phase_config_t *config,
    signal_dual_adc_phase_result_t *result);

#endif /* SIGNAL_DUAL_ADC_PHASE_H */
