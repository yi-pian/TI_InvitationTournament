#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "signal_autocorrelation.h"
#include "signal_correlation.h"
#include "signal_fft.h"
#include "signal_phase.h"
#include "test_helpers.h"

#define TEST_PI_F 3.14159265358979323846f

static void Test_PhaseAdapters(test_summary_t *summary)
{
    signal_phase_result_t result;
    const signal_complex_f32_t spectrum_a[] = {{1.0f, 0.0f}};
    const signal_complex_f32_t spectrum_b[] = {{0.0f, 1.0f}};

    Test_CheckU32(summary, "ZeroCross phase status",
        (unsigned int)SignalPhase_FromZeroCross(
            10.0f, 15.0f, 100.0f, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "ZeroCross B-A phase", result.phase_difference_deg,
                   -18.0f, 1.0e-6f);

    Test_CheckU32(summary, "FFT phasor status",
        (unsigned int)SignalPhase_FromFFTBin(
            spectrum_a, spectrum_b, 1U, 0U, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "FFT phasor B-A phase", result.phase_difference_deg,
                   90.0f, 1.0e-5f);

    Test_CheckU32(summary, "Lag phase status",
        (unsigned int)SignalPhase_FromCorrelationLag(
            25.0f, 100.0f, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Lag B-A phase", result.phase_difference_deg,
                   -90.0f, 1.0e-5f);
}

static void Test_FFTPhaseFullChain(test_summary_t *summary)
{
    float samples_a[256];
    float samples_b[256];
    signal_complex_f32_t spectrum_a[256];
    signal_complex_f32_t spectrum_b[256];
    signal_phase_result_t result;
    uint32_t index;
    const float phase_b_rad = 30.0f * TEST_PI_F / 180.0f;

    for (index = 0U; index < 256U; ++index)
    {
        float angle = 2.0f * TEST_PI_F * 10.0f * (float)index / 256.0f;
        samples_a[index] = sinf(angle + 0.1f);
        samples_b[index] = sinf(angle + 0.1f + phase_b_rad);
    }
    Test_CheckU32(summary, "FFT phase A transform",
        (unsigned int)SignalFFT_ForwardReal(samples_a, spectrum_a, 256U, 256U),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "FFT phase B transform",
        (unsigned int)SignalFFT_ForwardReal(samples_b, spectrum_b, 256U, 256U),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "FFT phase full status",
        (unsigned int)SignalPhase_FromFFTBin(
            spectrum_a, spectrum_b, 256U, 10U, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "FFT full B-A phase", result.phase_difference_deg,
                   30.0f, 2.0e-4f);
}

static void Test_CorrelationPhase(test_summary_t *summary)
{
    float samples_a[256];
    float samples_b[256];
    float coefficients[17];
    signal_correlation_result_t correlation_result;
    signal_phase_result_t phase_result;
    uint32_t index;

    for (index = 0U; index < 256U; ++index)
    {
        samples_a[index] = sinf(2.0f * TEST_PI_F * (float)index / 32.0f);
        samples_b[index] = sinf(
            2.0f * TEST_PI_F * ((float)index - 4.0f) / 32.0f);
    }
    Test_CheckU32(summary, "Correlation status",
        (unsigned int)SignalCorrelation_Process(
            samples_a, samples_b, 256U, 8U,
            coefficients, 17U, &correlation_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "Correlation best lag",
        (unsigned int)correlation_result.best_lag_samples, 4U);
    Test_CheckNear(summary, "Correlation peak", correlation_result.best_coefficient,
                   1.0f, 1.0e-5f);
    Test_CheckU32(summary, "Correlation phase status",
        (unsigned int)SignalPhase_FromCorrelationLag(
            (float)correlation_result.best_lag_samples,
            32.0f, &phase_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Correlation B-A phase",
                   phase_result.phase_difference_deg, -45.0f, 1.0e-5f);
}

static void Test_AutocorrelationFrequency(test_summary_t *summary)
{
    float samples[256];
    float coefficients[65];
    signal_autocorrelation_result_t correlation_result;
    signal_autocorrelation_period_result_t period_result;
    uint32_t index;

    for (index = 0U; index < 256U; ++index)
    {
        samples[index] = sinf(2.0f * TEST_PI_F * (float)index / 32.0f);
    }
    Test_CheckU32(summary, "Autocorrelation status",
        (unsigned int)SignalAutocorrelation_Process(
            samples, 256U, 64U, coefficients, 65U, &correlation_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Autocorrelation lag0", coefficients[0],
                   1.0f, 1.0e-6f);
    Test_CheckU32(summary, "Autocorrelation period status",
        (unsigned int)SignalAutocorrelation_FindPeriod(
            coefficients, correlation_result.lag_count,
            20U, 40U, 32000.0f, &period_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "Autocorrelation period lag",
                  period_result.period_lag_samples, 32U);
    Test_CheckNear(summary, "Autocorrelation frequency",
                   period_result.frequency_hz, 1000.0f, 1.0e-5f);
}

int main(void)
{
    test_summary_t summary = {0U, 0U};

    puts("=== MSPM0 Signal Algorithm PC Test: Sixth Batch ===");
    puts("Convention: phase_difference = phase_B - phase_A; positive lag means B is later");
    Test_PhaseAdapters(&summary);
    Test_FFTPhaseFullChain(&summary);
    Test_CorrelationPhase(&summary);
    Test_AutocorrelationFrequency(&summary);
    printf("=== SUMMARY: PASS=%u FAIL=%u ===\n", summary.passed, summary.failed);
    return (summary.failed == 0U) ? 0 : 1;
}
