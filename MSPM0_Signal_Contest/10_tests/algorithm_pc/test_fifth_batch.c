#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "signal_fft.h"
#include "signal_fft_magnitude.h"
#include "signal_hann.h"
#include "signal_harmonic.h"
#include "signal_multi_bin_energy.h"
#include "signal_sfdr.h"
#include "signal_snr.h"
#include "signal_thd.h"
#include "test_helpers.h"

#define TEST_PI_F 3.14159265358979323846f
#define TEST_N 1024U
#define TEST_BINS ((TEST_N / 2U) + 1U)

static float g_input[TEST_N];
static signal_complex_f32_t g_spectrum[TEST_N];
static float g_magnitude[TEST_BINS];

static void Test_MultiBin(test_summary_t *summary)
{
    const float magnitude[] = {0.0f, 3.0f, 4.0f, 0.0f};
    signal_multi_bin_energy_result_t result;

    Test_CheckU32(summary, "MultiBin status",
        (unsigned int)SignalMultiBinEnergy_Process(
            magnitude, 4U, 1U, 1U, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "MultiBin energy", result.energy, 25.0f, 1.0e-6f);
    Test_CheckNear(summary, "MultiBin RSS", result.root_sum_square, 5.0f, 1.0e-6f);
    Test_CheckU32(summary, "MultiBin start", result.start_bin, 0U);
    Test_CheckU32(summary, "MultiBin end", result.end_bin, 2U);
}

static void GenerateHarmonicSignal(float fundamental_hz, int use_hann)
{
    const float sample_rate_hz = 102400.0f;
    uint32_t index;
    signal_window_result_t window_result;

    for (index = 0U; index < TEST_N; ++index)
    {
        float time_s = (float)index / sample_rate_hz;
        g_input[index] =
            0.5f * sinf(2.0f * TEST_PI_F * fundamental_hz * time_s + 0.10f) +
            0.05f * sinf(2.0f * TEST_PI_F * 2.0f * fundamental_hz * time_s + 0.40f) +
            0.025f * sinf(2.0f * TEST_PI_F * 3.0f * fundamental_hz * time_s - 0.20f);
    }
    if (use_hann != 0)
    {
        (void)SignalHann_Apply(g_input, g_input, TEST_N, &window_result);
    }
    (void)SignalFFT_ForwardReal(g_input, g_spectrum, TEST_N, TEST_N);
    {
        signal_fft_magnitude_result_t magnitude_result;
        (void)SignalFFTMagnitude_Process(g_spectrum, TEST_N,
                                        g_magnitude, TEST_BINS,
                                        &magnitude_result);
    }
}

static void Test_HarmonicTHDBasic(test_summary_t *summary)
{
    const signal_harmonic_config_t config = {1000.0f, 1U, 3U, 0U};
    signal_harmonic_result_t harmonic_result;
    signal_thd_result_t thd_result;
    const float expected_thd_percent =
        100.0f * sqrtf((0.05f * 0.05f + 0.025f * 0.025f) /
                       (0.5f * 0.5f));

    GenerateHarmonicSignal(1000.0f, 0);
    Test_CheckU32(summary, "Harmonic BASIC status",
        (unsigned int)SignalHarmonic_Process(
            g_magnitude, TEST_BINS, 102400.0f, TEST_N,
            &config, &harmonic_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "Harmonic H1 bin",
                  harmonic_result.items[1].center_bin, 10U);
    Test_CheckU32(summary, "Harmonic H2 bin",
                  harmonic_result.items[2].center_bin, 20U);
    Test_CheckU32(summary, "Harmonic H3 bin",
                  harmonic_result.items[3].center_bin, 30U);
    Test_CheckU32(summary, "THD BASIC status",
        (unsigned int)SignalTHD_Process(&harmonic_result, &thd_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "THD BASIC percent", thd_result.thd_percent,
                   expected_thd_percent, 2.0e-3f);
}

static void Test_HarmonicTHDCompetition(test_summary_t *summary)
{
    const signal_harmonic_config_t config = {1037.0f, 1U, 3U, 2U};
    signal_harmonic_result_t harmonic_result;
    signal_thd_result_t thd_result;
    const float expected_thd_percent =
        100.0f * sqrtf((0.05f * 0.05f + 0.025f * 0.025f) /
                       (0.5f * 0.5f));

    GenerateHarmonicSignal(1037.0f, 1);
    Test_CheckU32(summary, "Harmonic COMP status",
        (unsigned int)SignalHarmonic_Process(
            g_magnitude, TEST_BINS, 102400.0f, TEST_N,
            &config, &harmonic_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "THD COMP status",
        (unsigned int)SignalTHD_Process(&harmonic_result, &thd_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "THD COMP percent", thd_result.thd_percent,
                   expected_thd_percent, 0.10f);
}

static void Test_SNRAndSFDR(test_summary_t *summary)
{
    const float magnitude[] = {0.0f, 1.0f, 10.0f, 1.0f, 2.0f,
                               1.0f, 1.0f, 1.0f, 1.0f};
    const signal_bin_range_t excluded[] = {{4U, 4U}};
    const signal_snr_config_t snr_config = {
        2U, 2U, 1U, 8U, excluded, 1U
    };
    const signal_sfdr_config_t sfdr_config = {2U, 2U, 1U, 8U};
    signal_snr_result_t snr_result;
    signal_sfdr_result_t sfdr_result;

    Test_CheckU32(summary, "SNR status",
        (unsigned int)SignalSNR_Process(
            magnitude, 9U, &snr_config, &snr_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "SNR signal energy", snr_result.signal_energy,
                   100.0f, 1.0e-6f);
    Test_CheckNear(summary, "SNR noise energy", snr_result.noise_energy,
                   6.0f, 1.0e-6f);
    Test_CheckNear(summary, "SNR dB", snr_result.snr_db,
                   10.0f * log10f(100.0f / 6.0f), 1.0e-5f);
    Test_CheckU32(summary, "SNR noise bin count", snr_result.noise_bin_count, 6U);

    Test_CheckU32(summary, "SFDR status",
        (unsigned int)SignalSFDR_Process(
            magnitude, 9U, &sfdr_config, &sfdr_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "SFDR main bin", sfdr_result.main_peak_bin, 2U);
    Test_CheckU32(summary, "SFDR spur bin", sfdr_result.spur_peak_bin, 4U);
    Test_CheckNear(summary, "SFDR dB", sfdr_result.sfdr_db,
                   20.0f * log10f(5.0f), 1.0e-5f);
}

int main(void)
{
    test_summary_t summary = {0U, 0U};

    puts("=== MSPM0 Signal Algorithm PC Test: Fifth Batch ===");
    puts("Truth: A1=0.5, A2=0.05, A3=0.025 -> THD=11.1803399%");
    Test_MultiBin(&summary);
    Test_HarmonicTHDBasic(&summary);
    Test_HarmonicTHDCompetition(&summary);
    Test_SNRAndSFDR(&summary);
    printf("=== SUMMARY: PASS=%u FAIL=%u ===\n", summary.passed, summary.failed);
    return (summary.failed == 0U) ? 0 : 1;
}
