#ifndef SIGNAL_FIXED_POINT_H
#define SIGNAL_FIXED_POINT_H

#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

/** Convert unsigned ADC codes to centered 1.15 values. */
signal_result_t SignalFixedPoint_AdcU16ToQ15(const uint16_t *input,
    size_t count, uint8_t adc_bits, int16_t *output, size_t output_capacity);

/** Convert 1.15 values to normalized float values in [-1.0, 1.0). */
signal_result_t SignalFixedPoint_Q15ToF32(const int16_t *input,
    size_t count, float *output, size_t output_capacity);

/** Convert 1.15 values to a signed 32-bit IQ value with 1..30 fraction bits. */
signal_result_t SignalFixedPoint_Q15ToIQ(const int16_t *input,
    size_t count, uint8_t iq_fraction_bits, int32_t *output,
    size_t output_capacity);

/** Convert signed 32-bit IQ values with 1..30 fraction bits to float. */
signal_result_t SignalFixedPoint_IQToF32(const int32_t *input,
    size_t count, uint8_t iq_fraction_bits, float *output,
    size_t output_capacity);

#endif
