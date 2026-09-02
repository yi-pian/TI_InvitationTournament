#ifndef SIGNAL_TEST_VECTORS_H
#define SIGNAL_TEST_VECTORS_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float sample_rate_hz;
    float frequency_hz;
    float amplitude_peak_v;
    float dc_offset_v;
    float phase_rad;
} signal_test_sine_config_t;

/** @brief 生成干净正弦，输入配置单位为 Hz/V/rad，输出单位 V。 */
signal_algorithm_status_t SignalTestVector_CleanSine(
    float *output_v, uint32_t count, const signal_test_sine_config_t *config);

/** @brief 生成带 config.dc_offset_v 的正弦，输出单位 V。 */
signal_algorithm_status_t SignalTestVector_SineWithDC(
    float *output_v, uint32_t count, const signal_test_sine_config_t *config);

/** @brief 生成带确定性均匀噪声的正弦；噪声峰值单位 V，相同 seed 可复现。 */
signal_algorithm_status_t SignalTestVector_NoisySine(
    float *output_v, uint32_t count, const signal_test_sine_config_t *config,
    float uniform_noise_peak_v, uint32_t seed);

/** @brief 生成含二、三次同相谐波的正弦；各谐波参数单位为峰值 V。 */
signal_algorithm_status_t SignalTestVector_SineWithHarmonics(
    float *output_v, uint32_t count, const signal_test_sine_config_t *config,
    float second_harmonic_peak_v, float third_harmonic_peak_v);

/** @brief 生成 50% 占空比双电平方波，峰值由 amplitude_peak_v 指定。 */
signal_algorithm_status_t SignalTestVector_SquareWave(
    float *output_v, uint32_t count, const signal_test_sine_config_t *config);

/** @brief 生成对称三角波，输出峰值与 config.amplitude_peak_v 一致。 */
signal_algorithm_status_t SignalTestVector_TriangleWave(
    float *output_v, uint32_t count, const signal_test_sine_config_t *config);

/** @brief 在干净正弦指定索引叠加一个单位 V 的 impulse_delta_v。 */
signal_algorithm_status_t SignalTestVector_ImpulseNoise(
    float *output_v, uint32_t count, const signal_test_sine_config_t *config,
    uint32_t impulse_index, float impulse_delta_v);

/** @brief 在主正弦上叠加第二个频率/峰值/相位明确的正弦。 */
signal_algorithm_status_t SignalTestVector_TwoTone(
    float *output_v, uint32_t count, const signal_test_sine_config_t *config,
    float second_frequency_hz, float second_amplitude_peak_v,
    float second_phase_rad);

/** @brief 只在 [start,end) 生成正弦，其余点保留 dc_offset_v。 */
signal_algorithm_status_t SignalTestVector_Burst(
    float *output_v, uint32_t count, const signal_test_sine_config_t *config,
    uint32_t burst_start_index, uint32_t burst_end_index);

/** @brief 生成正弦并按单位 V 的上下界进行硬削顶。 */
signal_algorithm_status_t SignalTestVector_ClippedSine(
    float *output_v, uint32_t count, const signal_test_sine_config_t *config,
    float lower_clip_v, float upper_clip_v);

#endif /* SIGNAL_TEST_VECTORS_H */
