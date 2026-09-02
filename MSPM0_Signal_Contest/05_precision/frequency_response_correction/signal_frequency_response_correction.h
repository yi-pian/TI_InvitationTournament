#ifndef SIGNAL_FREQUENCY_RESPONSE_CORRECTION_H
#define SIGNAL_FREQUENCY_RESPONSE_CORRECTION_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef enum
{
    SIGNAL_FRC_INTERPOLATE_LINEAR_HZ = 0,
    SIGNAL_FRC_INTERPOLATE_LOG_HZ = 1
} signal_frc_interpolation_t;

typedef enum
{
    SIGNAL_FRC_RANGE_REJECT = 0,
    SIGNAL_FRC_RANGE_CLAMP = 1
} signal_frc_range_policy_t;

typedef struct
{
    float frequency_hz;
    float gain_correction_linear;
    float phase_correction_deg;
} signal_frequency_response_correction_point_t;

typedef struct
{
    float corrected_gain_linear;
    float corrected_phase_deg;
    float applied_gain_correction_linear;
    float applied_phase_correction_deg;
    float interpolation_fraction;
    uint32_t lower_index;
    uint32_t upper_index;
} signal_frequency_response_correction_result_t;
/**
 * @brief 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。
 * @param table 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。
 * @param table_count 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。
 * @param frequency_hz 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。
 * @param measured_gain_linear `measured_gain_linear`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param measured_phase_deg `measured_phase_deg`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param interpolation `interpolation`（`signal_frc_interpolation_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param range_policy `range_policy`（`signal_frc_range_policy_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param result 由调用者分配的输出对象/数组。成功返回后才读取其中内容。
 * @return 返回 signal_algorithm_status_t 类型结果；调用者应检查该值。
 */

signal_algorithm_status_t SignalFrequencyResponseCorrection_Process(
    const signal_frequency_response_correction_point_t *table,
    uint32_t table_count,
    float frequency_hz,
    float measured_gain_linear,
    float measured_phase_deg,
    signal_frc_interpolation_t interpolation,
    signal_frc_range_policy_t range_policy,
    signal_frequency_response_correction_result_t *result);

#endif

