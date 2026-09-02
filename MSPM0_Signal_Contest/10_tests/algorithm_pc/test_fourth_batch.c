#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "signal_blackman.h"
#include "signal_fft.h"
#include "signal_fft_magnitude.h"
#include "signal_fft_parabolic_interpolation.h"
#include "signal_hamming.h"
#include "signal_hann.h"
#include "signal_log_parabolic_interpolation.h"
#include "signal_peak_detect.h"
#include "signal_rectangular.h"
#include "signal_window_gain_correction.h"
#include "test_helpers.h"

#define TEST_PI_F 3.14159265358979323846f
#define TEST_FFT_SIZE 1024U
#define TEST_BIN_COUNT ((TEST_FFT_SIZE / 2U) + 1U)

static float g_signal[TEST_FFT_SIZE];
static signal_complex_f32_t g_spectrum[TEST_FFT_SIZE];
static float g_magnitude[TEST_BIN_COUNT];
static float g_amplitude[TEST_BIN_COUNT];

static void Test_Windows(test_summary_t *summary)
{
    const float ones[8] = {1,1,1,1,1,1,1,1};
    float output[8];
    signal_window_result_t result;

    Test_CheckU32(summary, "Rectangular status",
        (unsigned int)SignalRectangular_Apply(ones, output, 8U, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Rectangular coherent gain",
                   result.coherent_gain, 1.0f, 1.0e-6f);
    Test_CheckNear(summary, "Rectangular endpoint", output[0], 1.0f, 1.0e-6f);

    Test_CheckU32(summary, "Hann status",
        (unsigned int)SignalHann_Apply(ones, output, 8U, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Hann first endpoint", output[0], 0.0f, 1.0e-6f);
    Test_CheckNear(summary, "Hann last endpoint", output[7], 0.0f, 1.0e-6f);
    Test_CheckNear(summary, "Hann coherent gain", result.coherent_gain,
                   7.0f / 16.0f, 1.0e-6f);

    Test_CheckU32(summary, "Hamming status",
        (unsigned int)SignalHamming_Apply(ones, output, 8U, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Hamming endpoint", output[0], 0.08f, 1.0e-6f);
    Test_CheckNear(summary, "Hamming coherent gain", result.coherent_gain,
                   ((0.54f * 8.0f) - 0.46f) / 8.0f, 1.0e-6f);

    Test_CheckU32(summary, "Blackman status",
        (unsigned int)SignalBlackman_Apply(ones, output, 8U, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Blackman endpoint", output[0], 0.0f, 1.0e-6f);
    Test_CheckNear(summary, "Blackman coherent gain", result.coherent_gain,
                   0.42f * 7.0f / 8.0f, 1.0e-6f);
}

static void Test_FFTImpulse(test_summary_t *summary)
{
    const float impulse[8] = {1,0,0,0,0,0,0,0};
    signal_complex_f32_t spectrum[8];
    float magnitude[5];
    signal_fft_magnitude_result_t magnitude_result;
    uint32_t bin;

    Test_CheckU32(summary, "FFT impulse status",
        (unsigned int)SignalFFT_ForwardReal(impulse, spectrum, 8U, 8U),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "Magnitude impulse status",
        (unsigned int)SignalFFTMagnitude_Process(
            spectrum, 8U, magnitude, 5U, &magnitude_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "Magnitude bin count", magnitude_result.bin_count, 5U);
    for (bin = 0U; bin < 5U; ++bin)
    {
        Test_CheckNear(summary, "FFT impulse magnitude", magnitude[bin],
                       1.0f, 2.0e-6f);
    }
}

static void Test_GainPeakAndParabola(test_summary_t *summary)
{
    float raw[513] = {0.0f};
    float corrected[513];
    signal_peak_detect_result_t peak_result;
    const float parabola[] = {0.0f, 8.4375f, 9.9375f, 9.4375f, 0.0f};
    signal_fft_parabolic_result_t parabolic_result;
    const float log_parabola[] = {
        0.1f, 0.209611387f, 0.939413063f, 0.569782825f, 0.1f};
    signal_log_parabolic_result_t log_parabolic_result;

    raw[10] = 256.0f;
    Test_CheckU32(summary, "Gain correction status",
        (unsigned int)SignalWindowGainCorrection_Apply(
            raw, corrected, 513U, 1024U, 1.0f),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Gain corrected peak", corrected[10],
                   0.5f, 1.0e-6f);

    Test_CheckU32(summary, "PeakDetect status",
        (unsigned int)SignalPeakDetect_Process(
            corrected, 513U, 1U, 512U, &peak_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "PeakDetect index", peak_result.peak_index, 10U);

    Test_CheckU32(summary, "Parabolic analytic status",
        (unsigned int)SignalFFTParabolicInterpolation_Process(
            parabola, 5U, 2U, 1000.0f, 100U, &parabolic_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Parabolic analytic offset",
                   parabolic_result.bin_offset, 0.25f, 1.0e-6f);
    Test_CheckNear(summary, "Parabolic analytic frequency",
                   parabolic_result.frequency_hz, 22.5f, 1.0e-5f);

    Test_CheckU32(summary, "Log-parabolic analytic status",
        (unsigned int)SignalLogParabolicInterpolation_Process(
            log_parabola, 5U, 2U, 1000.0f, 100U,
            &log_parabolic_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Log-parabolic analytic offset",
                   log_parabolic_result.bin_offset, 0.25f, 1.0e-6f);
    Test_CheckNear(summary, "Log-parabolic analytic frequency",
                   log_parabolic_result.frequency_hz, 22.5f, 1.0e-5f);
    Test_CheckNear(summary, "Log-parabolic analytic magnitude",
                   log_parabolic_result.interpolated_magnitude,
                   1.0f, 2.0e-6f);
}

static void Run_HannFFT(
    float frequency_hz,
    signal_window_result_t *window_result,
    signal_peak_detect_result_t *peak_result,
    signal_fft_parabolic_result_t *parabolic_result)
{
    const float sample_rate_hz = 102400.0f;
    signal_fft_magnitude_result_t magnitude_result;
    uint32_t index;

    for (index = 0U; index < TEST_FFT_SIZE; ++index)
    {
        g_signal[index] = 0.5f * sinf(
            2.0f * TEST_PI_F * frequency_hz *
            (float)index / sample_rate_hz + 0.31f);
    }
    (void)SignalHann_Apply(g_signal, g_signal, TEST_FFT_SIZE, window_result);
    (void)SignalFFT_ForwardReal(g_signal, g_spectrum,
                               TEST_FFT_SIZE, TEST_FFT_SIZE);
    (void)SignalFFTMagnitude_Process(g_spectrum, TEST_FFT_SIZE,
                                    g_magnitude, TEST_BIN_COUNT,
                                    &magnitude_result);
    (void)SignalWindowGainCorrection_Apply(
        g_magnitude, g_amplitude, TEST_BIN_COUNT,
        TEST_FFT_SIZE, window_result->coherent_gain);
    (void)SignalPeakDetect_Process(g_amplitude, TEST_BIN_COUNT,
                                   1U, TEST_BIN_COUNT - 2U, peak_result);
    (void)SignalFFTParabolicInterpolation_Process(
        g_magnitude, TEST_BIN_COUNT, peak_result->peak_index,
        sample_rate_hz, TEST_FFT_SIZE, parabolic_result);
}

static void Test_CompleteFFTChain(test_summary_t *summary)
{
    signal_window_result_t window_result;
    signal_peak_detect_result_t peak_result;
    signal_fft_parabolic_result_t parabolic_result;

    Run_HannFFT(1000.0f, &window_result, &peak_result, &parabolic_result);
    Test_CheckU32(summary, "FFT exact-bin peak index", peak_result.peak_index, 10U);
    Test_CheckNear(summary, "FFT exact-bin amplitude", g_amplitude[10],
                   0.5f, 1.0e-3f);
    Test_CheckNear(summary, "FFT exact-bin frequency",
                   parabolic_result.frequency_hz, 1000.0f, 0.2f);

    Run_HannFFT(1037.0f, &window_result, &peak_result, &parabolic_result);
    Test_CheckU32(summary, "FFT off-bin peak index", peak_result.peak_index, 10U);
    Test_CheckNear(summary, "FFT off-bin parabolic frequency",
                   parabolic_result.frequency_hz, 1037.0f, 6.0f);
}

int main(void)
{
    test_summary_t summary = {0U, 0U};

    puts("=== MSPM0 Signal Algorithm PC Test: Fourth Batch ===");
    puts("Truth: N=1024, Fs=102400 Hz, Hann, peak=0.5 V");
    Test_Windows(&summary);
    Test_FFTImpulse(&summary);
    Test_GainPeakAndParabola(&summary);
    Test_CompleteFFTChain(&summary);
    printf("=== SUMMARY: PASS=%u FAIL=%u ===\n", summary.passed, summary.failed);
    return (summary.failed == 0U) ? 0 : 1;
}
