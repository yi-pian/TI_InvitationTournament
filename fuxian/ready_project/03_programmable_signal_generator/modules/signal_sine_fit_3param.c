#include "signal_sine_fit_3param.h"

#include <math.h>
#include <stddef.h>

#define SIGNAL_SINE_FIT_PI_F 3.14159265358979323846f

static signal_algorithm_status_t SignalSineFit3_Solve(float matrix[3][4])
{
    uint32_t pivot_column;

    for (pivot_column = 0U; pivot_column < 3U; ++pivot_column)
    {
        uint32_t row;
        uint32_t pivot_row = pivot_column;
        float largest = fabsf(matrix[pivot_row][pivot_column]);
        for (row = pivot_column + 1U; row < 3U; ++row)
        {
            float candidate = fabsf(matrix[row][pivot_column]);
            if (candidate > largest)
            {
                largest = candidate;
                pivot_row = row;
            }
        }
        if (largest <= 1.0e-12f)
        {
            return SIGNAL_ALGORITHM_NO_FEATURE;
        }
        if (pivot_row != pivot_column)
        {
            uint32_t column;
            for (column = pivot_column; column < 4U; ++column)
            {
                float temporary = matrix[pivot_column][column];
                matrix[pivot_column][column] = matrix[pivot_row][column];
                matrix[pivot_row][column] = temporary;
            }
        }
        {
            float pivot = matrix[pivot_column][pivot_column];
            uint32_t column;
            for (column = pivot_column; column < 4U; ++column)
            {
                matrix[pivot_column][column] /= pivot;
            }
        }
        for (row = 0U; row < 3U; ++row)
        {
            if (row != pivot_column)
            {
                float factor = matrix[row][pivot_column];
                uint32_t column;
                for (column = pivot_column; column < 4U; ++column)
                {
                    matrix[row][column] -= factor * matrix[pivot_column][column];
                }
            }
        }
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalSineFit3Param_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_sine_fit_3param_config_t *config,
    signal_sine_fit_3param_result_t *result)
{
    float matrix[3][4] = {{0.0f}};
    float step;
    float step_cos;
    float step_sin;
    float cosine = 1.0f;
    float sine = 0.0f;
    uint32_t index;
    signal_algorithm_status_t status;
    float residual_sum = 0.0f;

    if ((voltage_v == NULL) || (config == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (count < 3U)
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    if (!isfinite(config->frequency_hz) ||
        !isfinite(config->sample_rate_hz) ||
        (config->frequency_hz <= 0.0f) ||
        (config->sample_rate_hz <= 0.0f) ||
        (config->frequency_hz >= (0.5f * config->sample_rate_hz)))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    step = 2.0f * SIGNAL_SINE_FIT_PI_F * config->frequency_hz /
           config->sample_rate_hz;
    step_cos = cosf(step);
    step_sin = sinf(step);

    for (index = 0U; index < count; ++index)
    {
        float value = voltage_v[index];
        float next_cosine;
        if (!isfinite(value))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        matrix[0][0] += cosine * cosine;
        matrix[0][1] += cosine * sine;
        matrix[0][2] += cosine;
        matrix[0][3] += cosine * value;
        matrix[1][1] += sine * sine;
        matrix[1][2] += sine;
        matrix[1][3] += sine * value;
        matrix[2][2] += 1.0f;
        matrix[2][3] += value;
        next_cosine = cosine * step_cos - sine * step_sin;
        sine = cosine * step_sin + sine * step_cos;
        cosine = next_cosine;
    }
    matrix[1][0] = matrix[0][1];
    matrix[2][0] = matrix[0][2];
    matrix[2][1] = matrix[1][2];
    status = SignalSineFit3_Solve(matrix);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    result->cosine_coefficient_v = matrix[0][3];
    result->sine_coefficient_v = matrix[1][3];
    result->dc_offset_v = matrix[2][3];
    result->amplitude_peak_v = hypotf(result->cosine_coefficient_v,
                                      result->sine_coefficient_v);
    result->phase_rad = atan2f(-result->sine_coefficient_v,
                              result->cosine_coefficient_v);
    result->phase_deg = result->phase_rad * 180.0f / SIGNAL_SINE_FIT_PI_F;

    cosine = 1.0f;
    sine = 0.0f;
    for (index = 0U; index < count; ++index)
    {
        float fitted = result->cosine_coefficient_v * cosine +
                       result->sine_coefficient_v * sine +
                       result->dc_offset_v;
        float residual = voltage_v[index] - fitted;
        float next_cosine = cosine * step_cos - sine * step_sin;
        residual_sum += residual * residual;
        sine = cosine * step_sin + sine * step_cos;
        cosine = next_cosine;
    }
    result->residual_rms_v = sqrtf(residual_sum / (float)count);
    return (isfinite(result->amplitude_peak_v) &&
            isfinite(result->phase_rad) &&
            isfinite(result->residual_rms_v))
               ? SIGNAL_ALGORITHM_OK
               : SIGNAL_ALGORITHM_NUMERIC_ERROR;
}
