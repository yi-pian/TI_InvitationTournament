#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "signal_cmsis_dsp_backend.h"
#include "signal_fixed_point.h"
#include "signal_math.h"
#include "signal_reference_backend.h"

#define BENCHMARK_MAX_FFT_SIZE 4096U

static signal_complex_f32_t g_reference[BENCHMARK_MAX_FFT_SIZE];
static int16_t g_q15[BENCHMARK_MAX_FFT_SIZE * 2U];
static int32_t g_q31[BENCHMARK_MAX_FFT_SIZE * 2U];
static float g_f32[BENCHMARK_MAX_FFT_SIZE * 2U];
static float g_real[BENCHMARK_MAX_FFT_SIZE];
static int16_t g_real_q15[BENCHMARK_MAX_FFT_SIZE];
static float g_magnitude_f32[BENCHMARK_MAX_FFT_SIZE];
static int16_t g_magnitude_q15[BENCHMARK_MAX_FFT_SIZE];

static float Benchmark_Input(size_t index, size_t count)
{
    float phase_a = SIGNAL_TWO_PI_F * 17.0f * (float) index / (float) count;
    float phase_b = SIGNAL_TWO_PI_F * 37.0f * (float) index / (float) count;
    return 0.35f * sinf(phase_a) + 0.15f * cosf(phase_b);
}

static int16_t Benchmark_ToQ15(float value)
{
    long converted = lroundf(value * 32768.0f);
    if (converted > INT16_MAX) { converted = INT16_MAX; }
    if (converted < INT16_MIN) { converted = INT16_MIN; }
    return (int16_t) converted;
}

static int32_t Benchmark_ToQ31(float value)
{
    double converted = (double) value * 2147483648.0;
    if (converted >= 2147483647.0) { return INT32_MAX; }
    if (converted <= -2147483648.0) { return INT32_MIN; }
    return (int32_t) llround(converted);
}

static void Benchmark_FillInputs(size_t count)
{
    size_t index;
    for (index = 0U; index < count; ++index) {
        float sample = Benchmark_Input(index, count);
        g_reference[index].real = sample;
        g_reference[index].imag = 0.0f;
        g_q15[2U * index] = Benchmark_ToQ15(sample);
        g_q15[2U * index + 1U] = 0;
        g_q31[2U * index] = Benchmark_ToQ31(sample);
        g_q31[2U * index + 1U] = 0;
        g_f32[2U * index] = sample;
        g_f32[2U * index + 1U] = 0.0f;
    }
}

static void Benchmark_PrintTransformResult(const char *operation,
    const char *backend, size_t count, size_t ram_bytes, float max_error,
    float rms_error, bool pass)
{
    printf("%s,%lu,%s,PENDING_BOARD,%lu,PENDING_TARGET_MAP,%.9g,%.9g,%s\n",
        operation, (unsigned long) count, backend,
        (unsigned long) ram_bytes, (double) max_error, (double) rms_error,
        pass ? "PASS" : "FAIL");
}

static bool Benchmark_RunFFTSize(size_t count)
{
    size_t index;
    double sum_error_q15 = 0.0;
    double sum_error_q31 = 0.0;
    double sum_error_f32 = 0.0;
    float max_error_q15 = 0.0f;
    float max_error_q31 = 0.0f;
    float max_error_f32 = 0.0f;
    bool forward_pass;
    signal_result_t result;

    Benchmark_FillInputs(count);
    result = SignalReference_FFTF32(g_reference, count, false);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalCMSISDSP_FFTQ15(g_q15, count, false);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalCMSISDSP_FFTQ31(g_q31, count, false);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalCMSISDSP_FFTF32(g_f32, count, false);
    if (result != SIGNAL_RESULT_OK) { return false; }

    for (index = 0U; index < count; ++index) {
        float expected_fixed_real = g_reference[index].real / (float) count;
        float expected_fixed_imag = g_reference[index].imag / (float) count;
        float q15_real = (float) g_q15[2U * index] / 32768.0f;
        float q15_imag = (float) g_q15[2U * index + 1U] / 32768.0f;
        float q31_real = (float) ((double) g_q31[2U * index] /
            2147483648.0);
        float q31_imag = (float) ((double) g_q31[2U * index + 1U] /
            2147483648.0);
        float error_q15_real = fabsf(q15_real - expected_fixed_real);
        float error_q15_imag = fabsf(q15_imag - expected_fixed_imag);
        float error_q31_real = fabsf(q31_real - expected_fixed_real);
        float error_q31_imag = fabsf(q31_imag - expected_fixed_imag);
        float error_f32_real = fabsf(g_f32[2U * index] -
            g_reference[index].real);
        float error_f32_imag = fabsf(g_f32[2U * index + 1U] -
            g_reference[index].imag);
        float pair_error_q15 = (error_q15_real > error_q15_imag) ?
            error_q15_real : error_q15_imag;
        float pair_error_q31 = (error_q31_real > error_q31_imag) ?
            error_q31_real : error_q31_imag;
        float pair_error_f32 = (error_f32_real > error_f32_imag) ?
            error_f32_real : error_f32_imag;
        if (pair_error_q15 > max_error_q15) { max_error_q15 = pair_error_q15; }
        if (pair_error_q31 > max_error_q31) { max_error_q31 = pair_error_q31; }
        if (pair_error_f32 > max_error_f32) { max_error_f32 = pair_error_f32; }
        sum_error_q15 += (double) error_q15_real * error_q15_real +
            (double) error_q15_imag * error_q15_imag;
        sum_error_q31 += (double) error_q31_real * error_q31_real +
            (double) error_q31_imag * error_q31_imag;
        sum_error_f32 += (double) error_f32_real * error_f32_real +
            (double) error_f32_imag * error_f32_imag;
    }

    Benchmark_PrintTransformResult("FFT", "REFERENCE_C_F32", count,
        count * sizeof(signal_complex_f32_t), 0.0f, 0.0f, true);
    Benchmark_PrintTransformResult("FFT", "CMSIS_DSP_Q15", count,
        count * 2U * sizeof(int16_t), max_error_q15,
        (float) sqrt(sum_error_q15 / (double) (2U * count)),
        max_error_q15 < 0.002f);
    Benchmark_PrintTransformResult("FFT", "CMSIS_DSP_Q31", count,
        count * 2U * sizeof(int32_t), max_error_q31,
        (float) sqrt(sum_error_q31 / (double) (2U * count)),
        max_error_q31 < 0.00001f);
    Benchmark_PrintTransformResult("FFT", "CMSIS_DSP_F32", count,
        count * 2U * sizeof(float), max_error_f32,
        (float) sqrt(sum_error_f32 / (double) (2U * count)),
        max_error_f32 < 0.001f);
    forward_pass = (max_error_q15 < 0.002f) &&
        (max_error_q31 < 0.00001f) && (max_error_f32 < 0.001f);
    result = SignalReference_FFTF32(g_reference, count, true);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalCMSISDSP_FFTQ15(g_q15, count, true);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalCMSISDSP_FFTQ31(g_q31, count, true);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalCMSISDSP_FFTF32(g_f32, count, true);
    if (result != SIGNAL_RESULT_OK) { return false; }

    sum_error_q15 = 0.0;
    sum_error_q31 = 0.0;
    sum_error_f32 = 0.0;
    max_error_q15 = 0.0f;
    max_error_q31 = 0.0f;
    max_error_f32 = 0.0f;
    for (index = 0U; index < count; ++index) {
        float expected_fixed = g_reference[index].real / (float) count;
        float q15_value = (float) g_q15[2U * index] / 32768.0f;
        float q31_value = (float) ((double) g_q31[2U * index] /
            2147483648.0);
        float error_q15 = fabsf(q15_value - expected_fixed);
        float error_q31 = fabsf(q31_value - expected_fixed);
        float error_f32 = fabsf(g_f32[2U * index] -
            g_reference[index].real);
        if (error_q15 > max_error_q15) { max_error_q15 = error_q15; }
        if (error_q31 > max_error_q31) { max_error_q31 = error_q31; }
        if (error_f32 > max_error_f32) { max_error_f32 = error_f32; }
        sum_error_q15 += (double) error_q15 * error_q15;
        sum_error_q31 += (double) error_q31 * error_q31;
        sum_error_f32 += (double) error_f32 * error_f32;
    }
    Benchmark_PrintTransformResult("IFFT", "REFERENCE_C_F32", count,
        count * sizeof(signal_complex_f32_t), 0.0f, 0.0f, true);
    Benchmark_PrintTransformResult("IFFT", "CMSIS_DSP_Q15", count,
        count * 2U * sizeof(int16_t), max_error_q15,
        (float) sqrt(sum_error_q15 / (double) count),
        max_error_q15 < 0.001f);
    Benchmark_PrintTransformResult("IFFT", "CMSIS_DSP_Q31", count,
        count * 2U * sizeof(int32_t), max_error_q31,
        (float) sqrt(sum_error_q31 / (double) count),
        max_error_q31 < 0.00001f);
    Benchmark_PrintTransformResult("IFFT", "CMSIS_DSP_F32", count,
        count * 2U * sizeof(float), max_error_f32,
        (float) sqrt(sum_error_f32 / (double) count),
        max_error_f32 < 0.001f);
    return forward_pass && (max_error_q15 < 0.001f) &&
        (max_error_q31 < 0.00001f) && (max_error_f32 < 0.001f);
}

static bool Benchmark_RunVectorAndScalar(void)
{
    size_t index;
    float reference_rms = 0.0f;
    float cmsis_rms_f32 = 0.0f;
    int16_t cmsis_rms_q15 = 0;
    float rms_error_f32;
    float rms_error_q15;
    float sqrt_result;
    float atan2_result;
    float magnitude_error_f32;
    float magnitude_error_q15;
    int16_t sqrt_q15;
    int16_t atan2_q13;
    int16_t sin_q15;
    int16_t cos_q15;
    float sin_error_f32;
    float cos_error_f32;
    float sin_error_q15;
    float cos_error_q15;
    double sum = 0.0;
    signal_result_t result;

    for (index = 0U; index < 1024U; ++index) {
        float sample = 0.5f * sinf(SIGNAL_TWO_PI_F * 13.0f *
            (float) index / 1024.0f);
        g_real[index] = sample;
        g_real_q15[index] = Benchmark_ToQ15(sample);
        sum += (double) sample * sample;
    }
    reference_rms = (float) sqrt(sum / 1024.0);
    result = SignalCMSISDSP_RMSF32(g_real, 1024U, &cmsis_rms_f32);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalCMSISDSP_RMSQ15(g_real_q15, 1024U, &cmsis_rms_q15);
    if (result != SIGNAL_RESULT_OK) { return false; }
    rms_error_f32 = fabsf(cmsis_rms_f32 - reference_rms);
    rms_error_q15 = fabsf((float) cmsis_rms_q15 / 32768.0f - reference_rms);
    printf("RMS,1024,CMSIS_DSP_F32,PENDING_BOARD,4096,PENDING_TARGET_MAP,%.9g,%.9g,%s\n",
        (double) rms_error_f32, (double) rms_error_f32,
        rms_error_f32 < 0.00001f ? "PASS" : "FAIL");
    printf("RMS,1024,CMSIS_DSP_Q15,PENDING_BOARD,2048,PENDING_TARGET_MAP,%.9g,%.9g,%s\n",
        (double) rms_error_q15, (double) rms_error_q15,
        rms_error_q15 < 0.0002f ? "PASS" : "FAIL");

    for (index = 0U; index < 1024U; ++index) {
        g_f32[2U * index] = 0.3f;
        g_f32[2U * index + 1U] = -0.4f;
        g_q15[2U * index] = Benchmark_ToQ15(0.3f);
        g_q15[2U * index + 1U] = Benchmark_ToQ15(-0.4f);
    }
    result = SignalCMSISDSP_MagnitudeF32(g_f32, 1024U, g_magnitude_f32,
        BENCHMARK_MAX_FFT_SIZE);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalCMSISDSP_MagnitudeQ15(g_q15, 1024U, g_magnitude_q15,
        BENCHMARK_MAX_FFT_SIZE);
    if (result != SIGNAL_RESULT_OK) { return false; }
    magnitude_error_f32 = fabsf(g_magnitude_f32[0] - 0.5f);
    magnitude_error_q15 = fabsf((float) g_magnitude_q15[0] / 16384.0f - 0.5f);
    printf("MAGNITUDE,1024,CMSIS_DSP_F32,PENDING_BOARD,12288,PENDING_TARGET_MAP,%.9g,%.9g,%s\n",
        (double) magnitude_error_f32, (double) magnitude_error_f32,
        magnitude_error_f32 < 0.00001f ? "PASS" : "FAIL");
    printf("MAGNITUDE,1024,CMSIS_DSP_Q15,PENDING_BOARD,6144,PENDING_TARGET_MAP,%.9g,%.9g,%s\n",
        (double) magnitude_error_q15, (double) magnitude_error_q15,
        magnitude_error_q15 < 0.0002f ? "PASS" : "FAIL");

    result = SignalCMSISDSP_SqrtF32(0.25f, &sqrt_result);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalCMSISDSP_Atan2F32(0.5f, 0.5f, &atan2_result);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalCMSISDSP_SqrtQ15(Benchmark_ToQ15(0.25f), &sqrt_q15);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalCMSISDSP_Atan2Q15(Benchmark_ToQ15(0.5f),
        Benchmark_ToQ15(0.5f), &atan2_q13);
    if (result != SIGNAL_RESULT_OK) { return false; }
    printf("SQRT,1,CMSIS_DSP_F32,PENDING_BOARD,0,PENDING_TARGET_MAP,%.9g,%.9g,%s\n",
        (double) fabsf(sqrt_result - 0.5f),
        (double) fabsf(sqrt_result - 0.5f),
        fabsf(sqrt_result - 0.5f) < 0.00001f ? "PASS" : "FAIL");
    printf("SQRT,1,CMSIS_DSP_Q15,PENDING_BOARD,0,PENDING_TARGET_MAP,%.9g,%.9g,%s\n",
        (double) fabsf((float) sqrt_q15 / 32768.0f - 0.5f),
        (double) fabsf((float) sqrt_q15 / 32768.0f - 0.5f),
        fabsf((float) sqrt_q15 / 32768.0f - 0.5f) < 0.0002f ? "PASS" : "FAIL");
    printf("ATAN2_PHASE,1,CMSIS_DSP_F32,PENDING_BOARD,0,PENDING_TARGET_MAP,%.9g,%.9g,%s\n",
        (double) fabsf(atan2_result - SIGNAL_PI_F / 4.0f),
        (double) fabsf(atan2_result - SIGNAL_PI_F / 4.0f),
        fabsf(atan2_result - SIGNAL_PI_F / 4.0f) < 0.0001f ? "PASS" : "FAIL");
    printf("ATAN2_PHASE,1,CMSIS_DSP_Q15,PENDING_BOARD,0,PENDING_TARGET_MAP,%.9g,%.9g,%s\n",
        (double) fabsf((float) atan2_q13 / 8192.0f - SIGNAL_PI_F / 4.0f),
        (double) fabsf((float) atan2_q13 / 8192.0f - SIGNAL_PI_F / 4.0f),
        fabsf((float) atan2_q13 / 8192.0f - SIGNAL_PI_F / 4.0f) < 0.001f ? "PASS" : "FAIL");
    sin_error_f32 = fabsf(SignalCMSISDSP_SinF32(SIGNAL_PI_F / 4.0f) -
        SignalReference_SinF32(SIGNAL_PI_F / 4.0f));
    cos_error_f32 = fabsf(SignalCMSISDSP_CosF32(SIGNAL_PI_F / 4.0f) -
        SignalReference_CosF32(SIGNAL_PI_F / 4.0f));
    sin_q15 = SignalCMSISDSP_SinQ15(INT16_C(4096));
    cos_q15 = SignalCMSISDSP_CosQ15(INT16_C(4096));
    sin_error_q15 = fabsf((float) sin_q15 / 32768.0f -
        SignalReference_SinF32(SIGNAL_PI_F / 4.0f));
    cos_error_q15 = fabsf((float) cos_q15 / 32768.0f -
        SignalReference_CosF32(SIGNAL_PI_F / 4.0f));
    printf("SINCOS,1,CMSIS_DSP_F32,PENDING_BOARD,0,PENDING_TARGET_MAP,%.9g,%.9g,%s\n",
        (double) ((sin_error_f32 > cos_error_f32) ? sin_error_f32 :
            cos_error_f32),
        sqrt(((double) sin_error_f32 * sin_error_f32 +
            (double) cos_error_f32 * cos_error_f32) / 2.0),
        (sin_error_f32 < 0.00001f) && (cos_error_f32 < 0.00001f) ?
            "PASS" : "FAIL");
    printf("SINCOS,1,CMSIS_DSP_Q15,PENDING_BOARD,0,PENDING_TARGET_MAP,%.9g,%.9g,%s\n",
        (double) ((sin_error_q15 > cos_error_q15) ? sin_error_q15 :
            cos_error_q15),
        sqrt(((double) sin_error_q15 * sin_error_q15 +
            (double) cos_error_q15 * cos_error_q15) / 2.0),
        (sin_error_q15 < 0.001f) && (cos_error_q15 < 0.001f) ?
            "PASS" : "FAIL");
    return (rms_error_f32 < 0.00001f) && (rms_error_q15 < 0.0002f) &&
        (magnitude_error_f32 < 0.00001f) &&
        (magnitude_error_q15 < 0.0002f) &&
        (sin_error_f32 < 0.00001f) && (cos_error_f32 < 0.00001f) &&
        (sin_error_q15 < 0.001f) && (cos_error_q15 < 0.001f);
}

static bool Benchmark_RunConversions(void)
{
    static const uint16_t adc_input[] = {0U, 2048U, 4095U};
    static const int16_t expected_q15[] = {INT16_MIN, 0, 32752};
    int16_t q15[3];
    int32_t iq24[3];
    float q15_f32[3];
    float iq_f32[3];
    size_t index;
    float max_error = 0.0f;
    signal_result_t result;

    result = SignalFixedPoint_AdcU16ToQ15(adc_input, 3U, 12U, q15, 3U);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalFixedPoint_Q15ToF32(q15, 3U, q15_f32, 3U);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalFixedPoint_Q15ToIQ(q15, 3U, 24U, iq24, 3U);
    if (result != SIGNAL_RESULT_OK) { return false; }
    result = SignalFixedPoint_IQToF32(iq24, 3U, 24U, iq_f32, 3U);
    if (result != SIGNAL_RESULT_OK) { return false; }

    for (index = 0U; index < 3U; ++index) {
        float error = fabsf(q15_f32[index] - iq_f32[index]);
        if (q15[index] != expected_q15[index]) { return false; }
        if (error > max_error) { max_error = error; }
    }
    printf("CONVERSION,3,REFERENCE_C,PENDING_BOARD,42,PENDING_TARGET_MAP,%.9g,%.9g,%s\n",
        (double) max_error, (double) max_error,
        max_error == 0.0f ? "PASS" : "FAIL");
    return max_error == 0.0f;
}

int main(void)
{
    static const size_t sizes[] = {512U, 1024U, 2048U, 4096U};
    size_t index;
    bool pass = true;
    printf("operation,size,backend,cycles,ram_bytes,flash_bytes,max_abs_error,rms_error,status\n");
    for (index = 0U; index < sizeof(sizes) / sizeof(sizes[0]); ++index) {
        if (!Benchmark_RunFFTSize(sizes[index])) { pass = false; }
    }
    if (!Benchmark_RunVectorAndScalar()) { pass = false; }
    if (!Benchmark_RunConversions()) { pass = false; }
    return pass ? 0 : 1;
}
