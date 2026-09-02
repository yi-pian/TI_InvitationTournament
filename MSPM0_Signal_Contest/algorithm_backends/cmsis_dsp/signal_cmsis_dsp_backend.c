#include "signal_cmsis_dsp_backend.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

/* CMSIS-DSP is deliberately isolated to this translation unit. */
#include "arm_const_structs.h"
#include "arm_math.h"

/* 0 keeps all audited lengths. A contest build may pin one length to reduce
 * linked twiddle tables without editing this source file. */
#ifndef SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE
#define SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE 0
#endif

#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE != 0) && \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE != 256) && \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE != 512) && \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE != 1024) && \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE != 2048) && \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE != 4096))
#error SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE must be 0, 256, 512, 1024, 2048 or 4096
#endif

static const arm_cfft_instance_q15 *SignalCMSISDSP_GetQ15Instance(
    size_t count)
{
    switch (count) {
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 256))
        case 256U: return &arm_cfft_sR_q15_len256;
#endif
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 512))
        case 512U: return &arm_cfft_sR_q15_len512;
#endif
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 1024))
        case 1024U: return &arm_cfft_sR_q15_len1024;
#endif
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 2048))
        case 2048U: return &arm_cfft_sR_q15_len2048;
#endif
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 4096))
        case 4096U: return &arm_cfft_sR_q15_len4096;
#endif
        default: return NULL;
    }
}

static const arm_cfft_instance_q31 *SignalCMSISDSP_GetQ31Instance(
    size_t count)
{
    switch (count) {
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 256))
        case 256U: return &arm_cfft_sR_q31_len256;
#endif
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 512))
        case 512U: return &arm_cfft_sR_q31_len512;
#endif
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 1024))
        case 1024U: return &arm_cfft_sR_q31_len1024;
#endif
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 2048))
        case 2048U: return &arm_cfft_sR_q31_len2048;
#endif
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 4096))
        case 4096U: return &arm_cfft_sR_q31_len4096;
#endif
        default: return NULL;
    }
}

static const arm_cfft_instance_f32 *SignalCMSISDSP_GetF32Instance(
    size_t count)
{
    switch (count) {
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 256))
        case 256U: return &arm_cfft_sR_f32_len256;
#endif
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 512))
        case 512U: return &arm_cfft_sR_f32_len512;
#endif
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 1024))
        case 1024U: return &arm_cfft_sR_f32_len1024;
#endif
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 2048))
        case 2048U: return &arm_cfft_sR_f32_len2048;
#endif
#if ((SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 0) || \
     (SIGNAL_CMSIS_DSP_FIXED_FFT_SIZE == 4096))
        case 4096U: return &arm_cfft_sR_f32_len4096;
#endif
        default: return NULL;
    }
}

static signal_result_t SignalCMSISDSP_CheckVector(const void *input,
    size_t count)
{
    if ((input == NULL) || (count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (count > (size_t) UINT32_MAX) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    return SIGNAL_RESULT_OK;
}

static signal_result_t SignalCMSISDSP_Status(arm_status status)
{
    switch (status) {
        case ARM_MATH_SUCCESS: return SIGNAL_RESULT_OK;
        case ARM_MATH_ARGUMENT_ERROR:
        case ARM_MATH_LENGTH_ERROR:
        case ARM_MATH_SIZE_MISMATCH:
            return SIGNAL_RESULT_INVALID_ARGUMENT;
        case ARM_MATH_NANINF:
        case ARM_MATH_SINGULAR:
        case ARM_MATH_TEST_FAILURE:
        case ARM_MATH_DECOMPOSITION_FAILURE:
            return SIGNAL_RESULT_NUMERIC_ERROR;
        default: return SIGNAL_RESULT_HARDWARE_ERROR;
    }
}

signal_result_t SignalCMSISDSP_FFTQ15(int16_t *interleaved,
    size_t complex_count, bool inverse)
{
    const arm_cfft_instance_q15 *instance;
    if (interleaved == NULL) { return SIGNAL_RESULT_INVALID_ARGUMENT; }
    instance = SignalCMSISDSP_GetQ15Instance(complex_count);
    if (instance == NULL) { return SIGNAL_RESULT_NOT_SUPPORTED; }
    arm_cfft_q15(instance, (q15_t *) interleaved, inverse ? 1U : 0U, 1U);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalCMSISDSP_FFTQ31(int32_t *interleaved,
    size_t complex_count, bool inverse)
{
    const arm_cfft_instance_q31 *instance;
    if (interleaved == NULL) { return SIGNAL_RESULT_INVALID_ARGUMENT; }
    instance = SignalCMSISDSP_GetQ31Instance(complex_count);
    if (instance == NULL) { return SIGNAL_RESULT_NOT_SUPPORTED; }
    arm_cfft_q31(instance, (q31_t *) interleaved, inverse ? 1U : 0U, 1U);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalCMSISDSP_FFTF32(float *interleaved,
    size_t complex_count, bool inverse)
{
    const arm_cfft_instance_f32 *instance;
    if (interleaved == NULL) { return SIGNAL_RESULT_INVALID_ARGUMENT; }
    instance = SignalCMSISDSP_GetF32Instance(complex_count);
    if (instance == NULL) { return SIGNAL_RESULT_NOT_SUPPORTED; }
    arm_cfft_f32(instance, (float32_t *) interleaved, inverse ? 1U : 0U, 1U);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalCMSISDSP_MagnitudeQ15(const int16_t *interleaved,
    size_t complex_count, int16_t *magnitude, size_t magnitude_capacity)
{
    signal_result_t result = SignalCMSISDSP_CheckVector(interleaved,
        complex_count);
    if (result != SIGNAL_RESULT_OK) { return result; }
    if (magnitude == NULL) { return SIGNAL_RESULT_INVALID_ARGUMENT; }
    if (magnitude_capacity < complex_count) {
        return SIGNAL_RESULT_INSUFFICIENT_BUFFER;
    }
    arm_cmplx_mag_q15((const q15_t *) interleaved, (q15_t *) magnitude,
        (uint32_t) complex_count);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalCMSISDSP_MagnitudeF32(const float *interleaved,
    size_t complex_count, float *magnitude, size_t magnitude_capacity)
{
    signal_result_t result = SignalCMSISDSP_CheckVector(interleaved,
        complex_count);
    if (result != SIGNAL_RESULT_OK) { return result; }
    if (magnitude == NULL) { return SIGNAL_RESULT_INVALID_ARGUMENT; }
    if (magnitude_capacity < complex_count) {
        return SIGNAL_RESULT_INSUFFICIENT_BUFFER;
    }
    arm_cmplx_mag_f32((const float32_t *) interleaved,
        (float32_t *) magnitude, (uint32_t) complex_count);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalCMSISDSP_RMSQ15(const int16_t *samples, size_t count,
    int16_t *rms)
{
    signal_result_t result = SignalCMSISDSP_CheckVector(samples, count);
    if (result != SIGNAL_RESULT_OK) { return result; }
    if (rms == NULL) { return SIGNAL_RESULT_INVALID_ARGUMENT; }
    arm_rms_q15((const q15_t *) samples, (uint32_t) count, (q15_t *) rms);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalCMSISDSP_RMSF32(const float *samples, size_t count,
    float *rms)
{
    signal_result_t result = SignalCMSISDSP_CheckVector(samples, count);
    if (result != SIGNAL_RESULT_OK) { return result; }
    if (rms == NULL) { return SIGNAL_RESULT_INVALID_ARGUMENT; }
    arm_rms_f32((const float32_t *) samples, (uint32_t) count,
        (float32_t *) rms);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalCMSISDSP_SqrtQ15(int16_t value, int16_t *result)
{
    if (result == NULL) { return SIGNAL_RESULT_INVALID_ARGUMENT; }
    return SignalCMSISDSP_Status(arm_sqrt_q15((q15_t) value,
        (q15_t *) result));
}

signal_result_t SignalCMSISDSP_Atan2Q15(int16_t y, int16_t x,
    int16_t *radians_q13)
{
    if (radians_q13 == NULL) { return SIGNAL_RESULT_INVALID_ARGUMENT; }
    return SignalCMSISDSP_Status(arm_atan2_q15((q15_t) y, (q15_t) x,
        (q15_t *) radians_q13));
}

int16_t SignalCMSISDSP_SinQ15(int16_t phase_q15)
{
    return (int16_t) arm_sin_q15((q15_t) phase_q15);
}

int16_t SignalCMSISDSP_CosQ15(int16_t phase_q15)
{
    return (int16_t) arm_cos_q15((q15_t) phase_q15);
}

signal_result_t SignalCMSISDSP_SqrtF32(float value, float *result)
{
    if (result == NULL) { return SIGNAL_RESULT_INVALID_ARGUMENT; }
    return SignalCMSISDSP_Status(arm_sqrt_f32(value, result));
}

signal_result_t SignalCMSISDSP_Atan2F32(float y, float x, float *radians)
{
    if (radians == NULL) { return SIGNAL_RESULT_INVALID_ARGUMENT; }
    return SignalCMSISDSP_Status(arm_atan2_f32(y, x, radians));
}

float SignalCMSISDSP_SinF32(float radians) { return arm_sin_f32(radians); }
float SignalCMSISDSP_CosF32(float radians) { return arm_cos_f32(radians); }
