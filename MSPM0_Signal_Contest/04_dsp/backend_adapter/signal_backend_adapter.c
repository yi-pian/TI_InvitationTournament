#include "signal_backend_adapter.h"

#include <math.h>
#include <stddef.h>

static int16_t SignalBackendAdapter_SaturateQ15(float normalized)
{
    if (normalized >= 1.0f)
    {
        return INT16_MAX;
    }
    if (normalized <= -1.0f)
    {
        return INT16_MIN;
    }
    if (normalized >= 0.0f)
    {
        return (int16_t)(normalized * 32767.0f + 0.5f);
    }
    return (int16_t)(normalized * 32768.0f - 0.5f);
}

signal_algorithm_status_t SignalBackendAdapter_FloatToQ15(
    const float *input_samples,
    int16_t *output_q15,
    uint32_t count,
    float full_scale)
{
    uint32_t index;

    if ((input_samples == NULL) || (output_q15 == NULL) ||
        (count == 0U) || !isfinite(full_scale) || (full_scale <= 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(input_samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }
    for (index = 0U; index < count; ++index)
    {
        output_q15[index] = SignalBackendAdapter_SaturateQ15(
            input_samples[index] / full_scale);
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalBackendAdapter_Q15ToFloat(
    const int16_t *input_q15,
    float *output_samples,
    uint32_t count,
    float full_scale)
{
    uint32_t index;

    if ((input_q15 == NULL) || (output_samples == NULL) ||
        (count == 0U) || !isfinite(full_scale) || (full_scale <= 0.0f))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (index = 0U; index < count; ++index)
    {
        output_samples[index] = ((float)input_q15[index] / 32768.0f) *
                                full_scale;
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalBackendAdapter_ADCRawToQ15(
    const uint16_t *adc_raw,
    int16_t *output_q15,
    uint32_t count,
    uint16_t zero_code,
    uint16_t positive_span_codes)
{
    uint32_t index;

    if ((adc_raw == NULL) || (output_q15 == NULL) ||
        (count == 0U) || (positive_span_codes == 0U))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (index = 0U; index < count; ++index)
    {
        int32_t centered_code = (int32_t)adc_raw[index] - (int32_t)zero_code;
        float normalized = (float)centered_code / (float)positive_span_codes;
        output_q15[index] = SignalBackendAdapter_SaturateQ15(normalized);
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalBackendAdapter_Q15SquareAccumulate(
    const int16_t *input_q15,
    uint32_t count,
    uint64_t *sum_squares_q30)
{
    uint32_t index;
    uint64_t sum = 0U;

    if ((input_q15 == NULL) || (sum_squares_q30 == NULL) || (count == 0U))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    for (index = 0U; index < count; ++index)
    {
        int32_t sample = input_q15[index];
        sum += (uint64_t)(sample * sample);
    }
    *sum_squares_q30 = sum;
    return SIGNAL_ALGORITHM_OK;
}
