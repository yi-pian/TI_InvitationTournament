#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "signal_backend_adapter.h"
#include "signal_fft.h"

#ifndef BENCH_BACKEND_NAME
#define BENCH_BACKEND_NAME "unknown"
#endif

#ifndef BENCH_NORMALIZED_TOLERANCE
#define BENCH_NORMALIZED_TOLERANCE 0.001f
#endif

#define BENCH_PI_D 3.1415926535897932384626433832795
#define BENCH_MAX_COUNT 4096U
#define BENCH_SIGNAL_COUNT 6U

typedef struct
{
    double real;
    double imag;
} benchmark_complex_f64_t;

static float input_samples[BENCH_MAX_COUNT];
static signal_complex_f32_t actual_spectrum[BENCH_MAX_COUNT];
static benchmark_complex_f64_t reference_spectrum[BENCH_MAX_COUNT];

static int BenchmarkIsPowerOfTwo(uint32_t value)
{
    return (value != 0U) && ((value & (value - 1U)) == 0U);
}

static void BenchmarkReferenceFFT(benchmark_complex_f64_t *data, uint32_t count)
{
    uint32_t index;
    uint32_t reversed = 0U;
    uint32_t length;

    if (!BenchmarkIsPowerOfTwo(count))
    {
        return;
    }
    for (index = 1U; index < count; ++index)
    {
        uint32_t bit = count >> 1U;
        while ((reversed & bit) != 0U)
        {
            reversed ^= bit;
            bit >>= 1U;
        }
        reversed ^= bit;
        if (index < reversed)
        {
            benchmark_complex_f64_t temporary = data[index];
            data[index] = data[reversed];
            data[reversed] = temporary;
        }
    }
    for (length = 2U; length <= count; length <<= 1U)
    {
        uint32_t block_start;
        uint32_t half_length = length >> 1U;
        double angle = -2.0 * BENCH_PI_D / (double)length;
        double step_real = cos(angle);
        double step_imag = sin(angle);
        for (block_start = 0U; block_start < count; block_start += length)
        {
            uint32_t offset;
            double twiddle_real = 1.0;
            double twiddle_imag = 0.0;
            for (offset = 0U; offset < half_length; ++offset)
            {
                uint32_t even_index = block_start + offset;
                uint32_t odd_index = even_index + half_length;
                double odd_real = data[odd_index].real * twiddle_real -
                                  data[odd_index].imag * twiddle_imag;
                double odd_imag = data[odd_index].real * twiddle_imag +
                                  data[odd_index].imag * twiddle_real;
                double even_real = data[even_index].real;
                double even_imag = data[even_index].imag;
                double next_twiddle_real;

                data[even_index].real = even_real + odd_real;
                data[even_index].imag = even_imag + odd_imag;
                data[odd_index].real = even_real - odd_real;
                data[odd_index].imag = even_imag - odd_imag;
                next_twiddle_real = twiddle_real * step_real -
                                    twiddle_imag * step_imag;
                twiddle_imag = twiddle_real * step_imag +
                               twiddle_imag * step_real;
                twiddle_real = next_twiddle_real;
            }
        }
        if (length == count)
        {
            break;
        }
    }
}

static float BenchmarkClip(float value, float lower, float upper)
{
    if (value < lower)
    {
        return lower;
    }
    if (value > upper)
    {
        return upper;
    }
    return value;
}

static void BenchmarkGenerateSignal(uint32_t signal_id, uint32_t count)
{
    uint32_t index;
    uint32_t noise_state = 0x13579BDFU;
    uint32_t fundamental_bin = 13U;

    for (index = 0U; index < count; ++index)
    {
        double angle = 2.0 * BENCH_PI_D * (double)fundamental_bin *
                       (double)index / (double)count;
        float sample;

        if (signal_id == 0U)
        {
            sample = 0.5f * (float)sin(angle);
        }
        else if (signal_id == 1U)
        {
            float noise;
            noise_state = noise_state * 1664525U + 1013904223U;
            noise = ((float)((noise_state >> 8U) & 0xFFFFU) / 32767.5f) - 1.0f;
            sample = 0.5f * (float)sin(angle) + 0.02f * noise;
        }
        else if (signal_id == 2U)
        {
            sample = 0.5f * (float)sin(angle) +
                     0.05f * (float)sin(2.0 * angle) +
                     0.025f * (float)sin(3.0 * angle);
        }
        else if (signal_id == 3U)
        {
            sample = 0.5f * (float)sin(angle) +
                     0.125f * (float)sin(2.0 * BENCH_PI_D * 31.0 *
                                         (double)index / (double)count);
        }
        else if (signal_id == 4U)
        {
            sample = 1.65f + 0.5f * (float)sin(angle);
        }
        else
        {
            sample = BenchmarkClip(0.7f * (float)sin(angle), -0.3f, 0.3f);
        }
        input_samples[index] = sample;
        reference_spectrum[index].real = (double)sample;
        reference_spectrum[index].imag = 0.0;
    }
}

static uint32_t BenchmarkFindPeakActual(uint32_t count)
{
    uint32_t index;
    uint32_t peak_index = 1U;
    double peak_power = -1.0;

    for (index = 1U; index < count / 2U; ++index)
    {
        double real = actual_spectrum[index].real;
        double imag = actual_spectrum[index].imag;
        double power = real * real + imag * imag;
        if (power > peak_power)
        {
            peak_power = power;
            peak_index = index;
        }
    }
    return peak_index;
}

static int BenchmarkRunFFTCase(uint32_t signal_id, uint32_t count)
{
    signal_algorithm_status_t status;
    double peak_reference = 0.0;
    double squared_error = 0.0;
    double maximum_error = 0.0;
    double normalized_rmse;
    double normalized_max;
    uint32_t index;
    uint32_t peak_index;
    int pass;

    BenchmarkGenerateSignal(signal_id, count);
    BenchmarkReferenceFFT(reference_spectrum, count);
    status = SignalFFT_ForwardReal(input_samples, actual_spectrum,
                                   count, BENCH_MAX_COUNT);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        printf("FFT,%s,%lu,%lu,status=%d,FAIL\n", BENCH_BACKEND_NAME,
               (unsigned long)count, (unsigned long)signal_id, (int)status);
        return 0;
    }
    for (index = 0U; index < count; ++index)
    {
        double reference_real = reference_spectrum[index].real;
        double reference_imag = reference_spectrum[index].imag;
        double error_real = (double)actual_spectrum[index].real - reference_real;
        double error_imag = (double)actual_spectrum[index].imag - reference_imag;
        double error = sqrt(error_real * error_real + error_imag * error_imag);
        double reference_magnitude = sqrt(reference_real * reference_real +
                                          reference_imag * reference_imag);
        squared_error += error * error;
        if (error > maximum_error)
        {
            maximum_error = error;
        }
        if (reference_magnitude > peak_reference)
        {
            peak_reference = reference_magnitude;
        }
    }
    normalized_rmse = sqrt(squared_error / (double)count) /
                      ((peak_reference > 0.0) ? peak_reference : 1.0);
    normalized_max = maximum_error /
                     ((peak_reference > 0.0) ? peak_reference : 1.0);
    peak_index = BenchmarkFindPeakActual(count);
    pass = (normalized_max <= (double)BENCH_NORMALIZED_TOLERANCE) &&
           (peak_index == 13U);
    printf("FFT,%s,%lu,%lu,rmse=%.9g,max=%.9g,peak=%lu,%s\n",
           BENCH_BACKEND_NAME, (unsigned long)count, (unsigned long)signal_id,
           normalized_rmse, normalized_max, (unsigned long)peak_index,
           pass ? "PASS" : "FAIL");
    return pass;
}

static int BenchmarkRunHannAmplitude(void)
{
    const uint32_t count = 1024U;
    const uint32_t bin = 10U;
    const float expected_amplitude = 0.5f;
    double window_sum = 0.0;
    double magnitude;
    double measured_amplitude;
    double absolute_error;
    uint32_t index;
    signal_algorithm_status_t status;
    int pass;

    for (index = 0U; index < count; ++index)
    {
        double window = 0.5 - 0.5 * cos(2.0 * BENCH_PI_D *
                                           (double)index / (double)(count - 1U));
        input_samples[index] = expected_amplitude *
                               (float)sin(2.0 * BENCH_PI_D * (double)bin *
                                          (double)index / (double)count) *
                               (float)window;
        window_sum += window;
    }
    status = SignalFFT_ForwardReal(input_samples, actual_spectrum,
                                   count, BENCH_MAX_COUNT);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        printf("HANN_AMPLITUDE,%s,status=%d,FAIL\n", BENCH_BACKEND_NAME,
               (int)status);
        return 0;
    }
    magnitude = hypot((double)actual_spectrum[bin].real,
                      (double)actual_spectrum[bin].imag);
    measured_amplitude = 2.0 * magnitude / window_sum;
    absolute_error = fabs(measured_amplitude - (double)expected_amplitude);
    pass = absolute_error <= (double)(BENCH_NORMALIZED_TOLERANCE * 2.0f);
    printf("HANN_AMPLITUDE,%s,expected=%.9g,measured=%.9g,abs=%.9g,%s\n",
           BENCH_BACKEND_NAME, (double)expected_amplitude,
           measured_amplitude, absolute_error, pass ? "PASS" : "FAIL");
    return pass;
}

static int BenchmarkRunAdapterTests(void)
{
    const uint16_t raw[5] = {0U, 1024U, 2048U, 3072U, 4095U};
    const float physical[5] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f};
    int16_t q15[5];
    float restored[5];
    uint64_t sum_squares = 0U;
    int pass = 1;

    pass &= SignalBackendAdapter_ADCRawToQ15(
                raw, q15, 5U, 2048U, 2048U) == SIGNAL_ALGORITHM_OK;
    pass &= (q15[0] == INT16_MIN) && (q15[2] == 0) && (q15[4] > 32740);
    pass &= SignalBackendAdapter_FloatToQ15(
                physical, q15, 5U, 1.0f) == SIGNAL_ALGORITHM_OK;
    pass &= (q15[0] == INT16_MIN) && (q15[1] == INT16_MIN) &&
            (q15[2] == 0) && (q15[3] == INT16_MAX) &&
            (q15[4] == INT16_MAX);
    pass &= SignalBackendAdapter_Q15ToFloat(
                q15, restored, 5U, 1.0f) == SIGNAL_ALGORITHM_OK;
    pass &= fabsf(restored[2]) < 1.0e-7f;
    pass &= SignalBackendAdapter_Q15SquareAccumulate(
                q15, 5U, &sum_squares) == SIGNAL_ALGORITHM_OK;
    pass &= sum_squares > 0U;
    printf("ADAPTER,%s,%s\n", BENCH_BACKEND_NAME, pass ? "PASS" : "FAIL");
    return pass;
}

int main(void)
{
    const uint32_t counts[4] = {512U, 1024U, 2048U, 4096U};
    uint32_t count_index;
    uint32_t signal_id;
    uint32_t pass_count = 0U;
    uint32_t fail_count = 0U;

    printf("BACKEND=%s,TOLERANCE=%.9g\n", BENCH_BACKEND_NAME,
           (double)BENCH_NORMALIZED_TOLERANCE);
    for (count_index = 0U; count_index < 4U; ++count_index)
    {
        for (signal_id = 0U; signal_id < BENCH_SIGNAL_COUNT; ++signal_id)
        {
            if (BenchmarkRunFFTCase(signal_id, counts[count_index]))
            {
                ++pass_count;
            }
            else
            {
                ++fail_count;
            }
        }
    }
    if (BenchmarkRunHannAmplitude())
    {
        ++pass_count;
    }
    else
    {
        ++fail_count;
    }
    if (BenchmarkRunAdapterTests())
    {
        ++pass_count;
    }
    else
    {
        ++fail_count;
    }
    printf("BACKEND_SUMMARY,%s,PASS=%lu,FAIL=%lu\n", BENCH_BACKEND_NAME,
           (unsigned long)pass_count, (unsigned long)fail_count);
    return (fail_count == 0U) ? 0 : 1;
}
