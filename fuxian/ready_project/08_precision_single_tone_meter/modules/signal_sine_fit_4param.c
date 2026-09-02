#include "signal_sine_fit_4param.h"

#include <math.h>
#include <stddef.h>

#define SIGNAL_SINE_FIT_GOLDEN_F 0.6180339887498948482f

static signal_algorithm_status_t SignalSineFit4_Evaluate(
    const float *voltage_v,
    uint32_t count,
    float sample_rate_hz,
    float frequency_hz,
    signal_sine_fit_3param_result_t *fit)
{
    signal_sine_fit_3param_config_t config;
    config.frequency_hz = frequency_hz;
    config.sample_rate_hz = sample_rate_hz;
    return SignalSineFit3Param_Process(voltage_v, count, &config, fit);
}

signal_algorithm_status_t SignalSineFit4Param_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_sine_fit_4param_config_t *config,
    signal_sine_fit_4param_result_t *result)
{
    float left;
    float right;
    float c_frequency;
    float d_frequency;
    signal_sine_fit_3param_result_t c_fit;
    signal_sine_fit_3param_result_t d_fit;
    uint32_t iteration;
    signal_algorithm_status_t status;

    if ((voltage_v == NULL) || (config == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count < 4U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(config->initial_frequency_hz) ||
        !isfinite(config->search_half_width_hz) ||
        !isfinite(config->sample_rate_hz) ||
        (config->initial_frequency_hz <= 0.0f) ||
        (config->search_half_width_hz <= 0.0f) ||
        (config->sample_rate_hz <= 0.0f) ||
        (config->iteration_count < 6U) ||
        (config->iteration_count > 40U))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    left = config->initial_frequency_hz - config->search_half_width_hz;
    right = config->initial_frequency_hz + config->search_half_width_hz;
    if ((left <= 0.0f) || (right >= (0.5f * config->sample_rate_hz)))
    {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    c_frequency = right - SIGNAL_SINE_FIT_GOLDEN_F * (right - left);
    d_frequency = left + SIGNAL_SINE_FIT_GOLDEN_F * (right - left);
    status = SignalSineFit4_Evaluate(
        voltage_v, count, config->sample_rate_hz, c_frequency, &c_fit);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    status = SignalSineFit4_Evaluate(
        voltage_v, count, config->sample_rate_hz, d_frequency, &d_fit);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }

    for (iteration = 0U; iteration < config->iteration_count; ++iteration)
    {
        if (c_fit.residual_rms_v < d_fit.residual_rms_v)
        {
            right = d_frequency;
            d_frequency = c_frequency;
            d_fit = c_fit;
            c_frequency = right - SIGNAL_SINE_FIT_GOLDEN_F * (right - left);
            status = SignalSineFit4_Evaluate(
                voltage_v, count, config->sample_rate_hz, c_frequency, &c_fit);
        }
        else
        {
            left = c_frequency;
            c_frequency = d_frequency;
            c_fit = d_fit;
            d_frequency = left + SIGNAL_SINE_FIT_GOLDEN_F * (right - left);
            status = SignalSineFit4_Evaluate(
                voltage_v, count, config->sample_rate_hz, d_frequency, &d_fit);
        }
        if (status != SIGNAL_ALGORITHM_OK)
        {
            return status;
        }
    }
    if (c_fit.residual_rms_v <= d_fit.residual_rms_v)
    {
        result->frequency_hz = c_frequency;
        result->waveform = c_fit;
    }
    else
    {
        result->frequency_hz = d_frequency;
        result->waveform = d_fit;
    }
    result->iteration_count = config->iteration_count;
    return SIGNAL_ALGORITHM_OK;
}
