#ifndef SIGNAL_IIR_BIQUAD_H
#define SIGNAL_IIR_BIQUAD_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
} signal_iir_biquad_coefficients_t;

typedef struct
{
    float d1;
    float d2;
} signal_iir_biquad_state_t;

typedef struct
{
    const signal_iir_biquad_coefficients_t *sections;
    signal_iir_biquad_state_t *states;
    uint32_t section_count;
    uint8_t initialized;
} signal_iir_biquad_t;

/**
 * @brief 初始化归一化 a0=1 的 Biquad/SOS 级联并清零状态。
 * @param instance IIR 实例。
 * @param sections 外部 SOS 系数数组，每节为 b0,b1,b2,a1,a2。
 * @param section_count 二阶节数量，必须大于 0。
 * @param states 调用者提供的状态数组，至少 section_count 项。
 * @param state_count 状态数组容量。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；不自动判断滤波器稳定性。
 */
signal_algorithm_status_t SignalIIRBiquad_Init(
    signal_iir_biquad_t *instance,
    const signal_iir_biquad_coefficients_t *sections,
    uint32_t section_count,
    signal_iir_biquad_state_t *states,
    uint32_t state_count);

/** @brief 清零全部 SOS 状态；instance 必须已初始化。 */
signal_algorithm_status_t SignalIIRBiquad_Reset(signal_iir_biquad_t *instance);

/**
 * @brief 用 Direct Form II Transposed 依次执行所有 Biquad 节。
 * @param instance 已初始化实例。
 * @param input_samples 输入数组，单位任意。
 * @param output_samples 输出数组，允许与输入为同一数组。
 * @param count 样本点数，必须大于 0。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；状态或数值错误返回对应码。
 * @note 系数决定截止频率和稳定性；本模块不写死低通/高通参数。
 */
signal_algorithm_status_t SignalIIRBiquad_Process(
    signal_iir_biquad_t *instance,
    const float *input_samples,
    float *output_samples,
    uint32_t count);

#endif /* SIGNAL_IIR_BIQUAD_H */
