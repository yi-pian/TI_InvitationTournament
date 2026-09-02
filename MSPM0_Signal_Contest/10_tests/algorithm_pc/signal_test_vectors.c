#include "signal_test_vectors.h"

#include <math.h>
#include <stddef.h>

#define SIGNAL_TEST_PI_F 3.14159265358979323846f

static signal_algorithm_status_t SignalTestVector_Validate(
    float *output_v,
    uint32_t count,
    const signal_test_sine_config_t *config)
{
    if ((output_v == NULL) || (config == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count == 0U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(config->sample_rate_hz) ||
        !isfinite(config->frequency_hz) ||
        !isfinite(config->amplitude_peak_v) ||
        !isfinite(config->dc_offset_v) ||
        !isfinite(config->phase_rad) ||
        (config->sample_rate_hz <= 0.0f) ||
        (config->frequency_hz <= 0.0f) ||
        (config->frequency_hz >= 0.5f * config->sample_rate_hz) ||
        (config->amplitude_peak_v < 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    return SIGNAL_ALGORITHM_OK;
}

static float SignalTestVector_Angle(
    uint32_t index,
    const signal_test_sine_config_t *config)
{
    return 2.0f * SIGNAL_TEST_PI_F * config->frequency_hz *
           (float)index / config->sample_rate_hz + config->phase_rad;
}

signal_algorithm_status_t SignalTestVector_CleanSine(
    float *output_v,
    uint32_t count,
    const signal_test_sine_config_t *config)
{
    uint32_t index;
    signal_algorithm_status_t status =
        SignalTestVector_Validate(output_v, count, config);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    for (index = 0U; index < count; ++index)
    {
        output_v[index] = config->dc_offset_v + config->amplitude_peak_v *
                          sinf(SignalTestVector_Angle(index, config));
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalTestVector_SineWithDC(
    float *output_v,
    uint32_t count,
    const signal_test_sine_config_t *config)
{
    return SignalTestVector_CleanSine(output_v, count, config);
}

signal_algorithm_status_t SignalTestVector_NoisySine(
    float *output_v,
    uint32_t count,
    const signal_test_sine_config_t *config,
    float uniform_noise_peak_v,
    uint32_t seed)
{
    uint32_t index;
    uint32_t state = seed;
    signal_algorithm_status_t status;

    if (!isfinite(uniform_noise_peak_v) || (uniform_noise_peak_v < 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    status = SignalTestVector_CleanSine(output_v, count, config);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    for (index = 0U; index < count; ++index)
    {
        float unit;
        state = state * 1664525U + 1013904223U;
        unit = (float)(state >> 8U) / 16777215.0f;
        output_v[index] += uniform_noise_peak_v * (2.0f * unit - 1.0f);
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalTestVector_SineWithHarmonics(
    float *output_v,
    uint32_t count,
    const signal_test_sine_config_t *config,
    float second_harmonic_peak_v,
    float third_harmonic_peak_v)
{
    uint32_t index;
    signal_algorithm_status_t status =
        SignalTestVector_Validate(output_v, count, config);
    if (!isfinite(second_harmonic_peak_v) ||
        !isfinite(third_harmonic_peak_v) ||
        (second_harmonic_peak_v < 0.0f) ||
        (third_harmonic_peak_v < 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    if ((3.0f * config->frequency_hz) >= 0.5f * config->sample_rate_hz)
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    for (index = 0U; index < count; ++index)
    {
        float angle = SignalTestVector_Angle(index, config);
        output_v[index] = config->dc_offset_v +
            config->amplitude_peak_v * sinf(angle) +
            second_harmonic_peak_v * sinf(2.0f * angle) +
            third_harmonic_peak_v * sinf(3.0f * angle);
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalTestVector_SquareWave(
    float *output_v,
    uint32_t count,
    const signal_test_sine_config_t *config)
{
    uint32_t index;
    signal_algorithm_status_t status =
        SignalTestVector_Validate(output_v, count, config);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    for (index = 0U; index < count; ++index)
    {
        float sign = (sinf(SignalTestVector_Angle(index, config)) >= 0.0f)
                         ? 1.0f : -1.0f;
        output_v[index] = config->dc_offset_v +
                          config->amplitude_peak_v * sign;
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalTestVector_TriangleWave(
    float *output_v,
    uint32_t count,
    const signal_test_sine_config_t *config)
{
    uint32_t index;
    signal_algorithm_status_t status =
        SignalTestVector_Validate(output_v, count, config);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    for (index = 0U; index < count; ++index)
    {
        output_v[index] = config->dc_offset_v +
            (2.0f * config->amplitude_peak_v / SIGNAL_TEST_PI_F) *
            asinf(sinf(SignalTestVector_Angle(index, config)));
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalTestVector_ImpulseNoise(
    float *output_v,
    uint32_t count,
    const signal_test_sine_config_t *config,
    uint32_t impulse_index,
    float impulse_delta_v)
{
    signal_algorithm_status_t status;
    if (!isfinite(impulse_delta_v) || (impulse_index >= count))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    status = SignalTestVector_CleanSine(output_v, count, config);
    if (status == SIGNAL_ALGORITHM_OK)
    {
        output_v[impulse_index] += impulse_delta_v;
    }
    return status;
}

signal_algorithm_status_t SignalTestVector_TwoTone(
    float *output_v,
    uint32_t count,
    const signal_test_sine_config_t *config,
    float second_frequency_hz,
    float second_amplitude_peak_v,
    float second_phase_rad)
{
    uint32_t index;
    signal_algorithm_status_t status =
        SignalTestVector_CleanSine(output_v, count, config);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    if (!isfinite(second_frequency_hz) ||
        !isfinite(second_amplitude_peak_v) ||
        !isfinite(second_phase_rad) ||
        (second_frequency_hz <= 0.0f) ||
        (second_amplitude_peak_v < 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (second_frequency_hz >= 0.5f * config->sample_rate_hz)
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    for (index = 0U; index < count; ++index)
    {
        float angle = 2.0f * SIGNAL_TEST_PI_F * second_frequency_hz *
                      (float)index / config->sample_rate_hz + second_phase_rad;
        output_v[index] += second_amplitude_peak_v * sinf(angle);
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalTestVector_Burst(
    float *output_v,
    uint32_t count,
    const signal_test_sine_config_t *config,
    uint32_t burst_start_index,
    uint32_t burst_end_index)
{
    uint32_t index;
    signal_algorithm_status_t status =
        SignalTestVector_Validate(output_v, count, config);
    if ((burst_start_index >= burst_end_index) || (burst_end_index > count))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    for (index = 0U; index < count; ++index)
    {
        float waveform = 0.0f;
        if ((index >= burst_start_index) && (index < burst_end_index))
        {
            waveform = config->amplitude_peak_v *
                       sinf(SignalTestVector_Angle(index, config));
        }
        output_v[index] = config->dc_offset_v + waveform;
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalTestVector_ClippedSine(
    float *output_v,
    uint32_t count,
    const signal_test_sine_config_t *config,
    float lower_clip_v,
    float upper_clip_v)
{
    uint32_t index;
    signal_algorithm_status_t status;
    if (!isfinite(lower_clip_v) || !isfinite(upper_clip_v) ||
        (lower_clip_v >= upper_clip_v))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    status = SignalTestVector_CleanSine(output_v, count, config);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    for (index = 0U; index < count; ++index)
    {
        if (output_v[index] < lower_clip_v)
        {
            output_v[index] = lower_clip_v;
        }
        else if (output_v[index] > upper_clip_v)
        {
            output_v[index] = upper_clip_v;
        }
    }
    return SIGNAL_ALGORITHM_OK;
}
