#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "signal_ac_rms.h"
#include "signal_adc_to_voltage.h"
#include "signal_clipping_detect.h"
#include "signal_mean.h"
#include "signal_minmax.h"
#include "signal_remove_dc.h"
#include "signal_rms.h"
#include "signal_statistics.h"
#include "signal_vpp.h"
#include "test_helpers.h"

#define TEST_SINE_COUNT 1000U
#define TEST_PI_F 3.14159265358979323846f

static void Test_ADCToVoltage(test_summary_t *summary)
{
    const uint16_t raw_codes[] = {0U, 2048U, 4095U};
    float voltage_v[3] = {0.0f, 0.0f, 0.0f};
    const signal_adc_to_voltage_config_t config = {
        4095U, 3.3f, 1.0f, 0.0f
    };
    signal_algorithm_status_t status = SignalADCToVoltage_Process(
        raw_codes, voltage_v, 3U, &config);

    Test_CheckU32(summary, "ADCToVoltage status", (unsigned int)status,
                  (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "ADCToVoltage code 0", voltage_v[0], 0.0f, 1.0e-7f);
    Test_CheckNear(summary, "ADCToVoltage code 2048", voltage_v[1],
                   (2048.0f * 3.3f) / 4095.0f, 1.0e-6f);
    Test_CheckNear(summary, "ADCToVoltage code 4095", voltage_v[2], 3.3f, 1.0e-6f);
}

static void Test_BasicStatistics(test_summary_t *summary)
{
    const float samples[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    signal_mean_result_t mean_result;
    signal_minmax_result_t minmax_result;
    signal_statistics_result_t statistics_result;

    Test_CheckU32(summary, "Mean status",
        (unsigned int)SignalMean_Process(samples, 5U, &mean_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Mean", mean_result.mean_value, 3.0f, 1.0e-6f);

    Test_CheckU32(summary, "MinMax status",
        (unsigned int)SignalMinMax_Process(samples, 5U, &minmax_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "MinMax minimum", minmax_result.min_value, 1.0f, 1.0e-6f);
    Test_CheckNear(summary, "MinMax maximum", minmax_result.max_value, 5.0f, 1.0e-6f);
    Test_CheckU32(summary, "MinMax minimum index", minmax_result.min_index, 0U);
    Test_CheckU32(summary, "MinMax maximum index", minmax_result.max_index, 4U);

    Test_CheckU32(summary, "Statistics status",
        (unsigned int)SignalStatistics_Process(samples, 5U, &statistics_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Statistics mean", statistics_result.mean_value, 3.0f, 1.0e-6f);
    Test_CheckNear(summary, "Statistics population var",
                   statistics_result.population_variance, 2.0f, 1.0e-6f);
    Test_CheckNear(summary, "Statistics sample var",
                   statistics_result.sample_variance, 2.5f, 1.0e-6f);
    Test_CheckNear(summary, "Statistics population std",
                   statistics_result.population_stddev, sqrtf(2.0f), 1.0e-6f);
}

static void Test_SineMeasurements(test_summary_t *summary)
{
    float voltage_v[TEST_SINE_COUNT];
    float centered_v[TEST_SINE_COUNT];
    uint32_t index;
    signal_vpp_result_t vpp_result;
    signal_rms_result_t rms_result;
    signal_ac_rms_result_t ac_rms_result;
    signal_remove_dc_result_t remove_dc_result;
    signal_mean_result_t centered_mean;
    const float expected_total_rms = sqrtf((1.65f * 1.65f) + (0.5f * 0.5f / 2.0f));

    /* Fs=100 kHz, f=1 kHz, 1000 点恰好覆盖 10 个周期。 */
    for (index = 0U; index < TEST_SINE_COUNT; ++index)
    {
        voltage_v[index] = 1.65f +
            0.5f * sinf(2.0f * TEST_PI_F * 1000.0f * (float)index / 100000.0f);
    }

    Test_CheckU32(summary, "VPP status",
        (unsigned int)SignalVPP_Process(voltage_v, TEST_SINE_COUNT, &vpp_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Sine Vpp", vpp_result.amplitude_vpp, 1.0f, 1.0e-5f);

    Test_CheckU32(summary, "RMS status",
        (unsigned int)SignalRMS_Process(voltage_v, TEST_SINE_COUNT, &rms_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Sine total RMS", rms_result.rms_v,
                   expected_total_rms, 2.0e-6f);

    Test_CheckU32(summary, "AC RMS status",
        (unsigned int)SignalACRMS_Process(voltage_v, TEST_SINE_COUNT, &ac_rms_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Sine mean voltage", ac_rms_result.mean_voltage_v,
                   1.65f, 2.0e-6f);
    Test_CheckNear(summary, "Sine AC RMS", ac_rms_result.ac_rms_v,
                   0.3535533906f, 2.0e-6f);

    Test_CheckU32(summary, "RemoveDC status",
        (unsigned int)SignalRemoveDC_Process(voltage_v, centered_v,
                                             TEST_SINE_COUNT, &remove_dc_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "RemoveDC removed mean", remove_dc_result.removed_mean_v,
                   1.65f, 2.0e-6f);
    (void)SignalMean_Process(centered_v, TEST_SINE_COUNT, &centered_mean);
    Test_CheckNear(summary, "RemoveDC output mean", centered_mean.mean_value,
                   0.0f, 3.0e-7f);
}

static void Test_ClippingAndErrors(test_summary_t *summary)
{
    const float voltage_v[] = {-0.01f, 0.00f, 1.65f, 3.29f, 3.31f};
    const signal_clipping_detect_config_t config = {0.02f, 3.28f};
    signal_clipping_detect_result_t result;
    const uint16_t invalid_raw[] = {4096U};
    float output_v = -123.0f;
    const signal_adc_to_voltage_config_t adc_config = {
        4095U, 3.3f, 1.0f, 0.0f
    };

    Test_CheckU32(summary, "ClippingDetect status",
        (unsigned int)SignalClippingDetect_Process(voltage_v, 5U, &config, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "Clipping low count", result.low_clipped_count, 2U);
    Test_CheckU32(summary, "Clipping high count", result.high_clipped_count, 2U);
    Test_CheckNear(summary, "Clipping ratio", result.clipped_ratio, 0.8f, 1.0e-6f);

    Test_CheckU32(summary, "ADC out-of-range status",
        (unsigned int)SignalADCToVoltage_Process(invalid_raw, &output_v, 1U, &adc_config),
        (unsigned int)SIGNAL_ALGORITHM_OUT_OF_RANGE);
    Test_CheckNear(summary, "ADC error keeps output", output_v, -123.0f, 0.0f);
    Test_CheckU32(summary, "Mean zero-count status",
        (unsigned int)SignalMean_Process(voltage_v, 0U, NULL),
        (unsigned int)SIGNAL_ALGORITHM_INVALID_ARGUMENT);
}

int main(void)
{
    test_summary_t summary = {0U, 0U};

    puts("=== MSPM0 Signal Algorithm PC Test: First Batch ===");
    puts("Truth sine: Fs=100000 Hz, f=1000 Hz, peak=0.5 V, DC=1.65 V");
    puts("Expected: Vpp=1.0 V, AC RMS=0.3535533906 V");

    Test_ADCToVoltage(&summary);
    Test_BasicStatistics(&summary);
    Test_SineMeasurements(&summary);
    Test_ClippingAndErrors(&summary);

    printf("=== SUMMARY: PASS=%u FAIL=%u ===\n", summary.passed, summary.failed);
    return (summary.failed == 0U) ? 0 : 1;
}
