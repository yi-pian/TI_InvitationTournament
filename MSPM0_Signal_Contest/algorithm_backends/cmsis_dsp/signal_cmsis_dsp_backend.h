#ifndef SIGNAL_CMSIS_DSP_BACKEND_H
#define SIGNAL_CMSIS_DSP_BACKEND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

/* Complex buffers are interleaved real, imag and transformed in place.
 * Q15/Q31 transforms apply the CMSIS fixed-point stage scaling. F32 forward
 * is unscaled; F32 inverse includes 1/N normalization. */
signal_result_t SignalCMSISDSP_FFTQ15(int16_t *interleaved,
    size_t complex_count, bool inverse);
signal_result_t SignalCMSISDSP_FFTQ31(int32_t *interleaved,
    size_t complex_count, bool inverse);
signal_result_t SignalCMSISDSP_FFTF32(float *interleaved,
    size_t complex_count, bool inverse);

signal_result_t SignalCMSISDSP_MagnitudeQ15(const int16_t *interleaved,
    size_t complex_count, int16_t *magnitude, size_t magnitude_capacity);
/* Q15 complex magnitude output is CMSIS 2.14, not ordinary 1.15. */
signal_result_t SignalCMSISDSP_MagnitudeF32(const float *interleaved,
    size_t complex_count, float *magnitude, size_t magnitude_capacity);
signal_result_t SignalCMSISDSP_RMSQ15(const int16_t *samples, size_t count,
    int16_t *rms);
signal_result_t SignalCMSISDSP_RMSF32(const float *samples, size_t count,
    float *rms);

signal_result_t SignalCMSISDSP_SqrtQ15(int16_t value, int16_t *result);
signal_result_t SignalCMSISDSP_Atan2Q15(int16_t y, int16_t x,
    int16_t *radians_q13);
/* Sin/cos input maps Q15 [0, 1) to phase [0, 2*pi). */
int16_t SignalCMSISDSP_SinQ15(int16_t phase_q15);
int16_t SignalCMSISDSP_CosQ15(int16_t phase_q15);

signal_result_t SignalCMSISDSP_SqrtF32(float value, float *result);
signal_result_t SignalCMSISDSP_Atan2F32(float y, float x, float *radians);
float SignalCMSISDSP_SinF32(float radians);
float SignalCMSISDSP_CosF32(float radians);

#endif
