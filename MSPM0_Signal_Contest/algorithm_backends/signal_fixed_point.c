#include "signal_fixed_point.h"

#include <stddef.h>
#include <stdint.h>

static signal_result_t SignalFixedPoint_CheckArguments(const void *input,
    size_t count, const void *output, size_t output_capacity)
{
    if ((input == NULL) || (output == NULL) || (count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (output_capacity < count) {
        return SIGNAL_RESULT_INSUFFICIENT_BUFFER;
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalFixedPoint_AdcU16ToQ15(const uint16_t *input,
    size_t count, uint8_t adc_bits, int16_t *output, size_t output_capacity)
{
    size_t index;
    uint32_t maximum_code;
    uint32_t midpoint;
    uint8_t left_shift;
    signal_result_t result = SignalFixedPoint_CheckArguments(input, count,
        output, output_capacity);
    if (result != SIGNAL_RESULT_OK) { return result; }
    if ((adc_bits < 2U) || (adc_bits > 16U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    maximum_code = (adc_bits == 16U) ? UINT16_MAX :
        ((UINT32_C(1) << adc_bits) - 1U);
    midpoint = UINT32_C(1) << (adc_bits - 1U);
    left_shift = (uint8_t) (16U - adc_bits);
    for (index = 0U; index < count; ++index) {
        if ((uint32_t) input[index] > maximum_code) {
            return SIGNAL_RESULT_OUT_OF_RANGE;
        }
    }
    for (index = 0U; index < count; ++index) {
        int32_t centered = (int32_t) input[index] - (int32_t) midpoint;
        output[index] = (int16_t) (centered *
            (int32_t) (UINT32_C(1) << left_shift));
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalFixedPoint_Q15ToF32(const int16_t *input,
    size_t count, float *output, size_t output_capacity)
{
    size_t index;
    signal_result_t result = SignalFixedPoint_CheckArguments(input, count,
        output, output_capacity);
    if (result != SIGNAL_RESULT_OK) { return result; }
    for (index = 0U; index < count; ++index) {
        output[index] = (float) input[index] / 32768.0f;
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalFixedPoint_Q15ToIQ(const int16_t *input,
    size_t count, uint8_t iq_fraction_bits, int32_t *output,
    size_t output_capacity)
{
    size_t index;
    int64_t iq_scale;
    signal_result_t result = SignalFixedPoint_CheckArguments(input, count,
        output, output_capacity);
    if (result != SIGNAL_RESULT_OK) { return result; }
    if ((iq_fraction_bits < 1U) || (iq_fraction_bits > 30U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    iq_scale = INT64_C(1) << iq_fraction_bits;
    for (index = 0U; index < count; ++index) {
        output[index] = (int32_t) (((int64_t) input[index] * iq_scale) /
            INT64_C(32768));
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalFixedPoint_IQToF32(const int32_t *input,
    size_t count, uint8_t iq_fraction_bits, float *output,
    size_t output_capacity)
{
    size_t index;
    uint32_t iq_scale;
    signal_result_t result = SignalFixedPoint_CheckArguments(input, count,
        output, output_capacity);
    if (result != SIGNAL_RESULT_OK) { return result; }
    if ((iq_fraction_bits < 1U) || (iq_fraction_bits > 30U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    iq_scale = UINT32_C(1) << iq_fraction_bits;
    for (index = 0U; index < count; ++index) {
        output[index] = (float) input[index] / (float) iq_scale;
    }
    return SIGNAL_RESULT_OK;
}
