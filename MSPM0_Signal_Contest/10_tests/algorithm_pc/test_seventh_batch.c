#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "signal_adc_gain_offset_calibration.h"
#include "signal_channel_delay_calibration.h"
#include "signal_lock_in.h"
#include "signal_robust_peak_to_peak.h"
#include "signal_robust_rms.h"
#include "signal_sine_fit_3param.h"
#include "signal_sine_fit_4param.h"
#include "test_helpers.h"

#define TEST_PI_F 3.14159265358979323846f

static void Test_ADCCalibration(test_summary_t *summary)
{
    signal_adc_gain_offset_calibration_t calibration;
    const float input_voltage_v[2] = {0.1f, 3.2f};
    float output_voltage_v[2];

    Test_CheckU32(summary, "ADC calibration compute status",
        (unsigned int)SignalADCGainOffsetCalibration_Compute(
            0.1f, 0.0f, 3.2f, 3.3f, &calibration),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "ADC calibration gain", calibration.gain,
                   3.3f / 3.1f, 1.0e-6f);
    Test_CheckNear(summary, "ADC calibration offset", calibration.offset_v,
                   -0.1f * 3.3f / 3.1f, 1.0e-6f);
    Test_CheckU32(summary, "ADC calibration apply status",
        (unsigned int)SignalADCGainOffsetCalibration_Apply(
            input_voltage_v, output_voltage_v, 2U, &calibration),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "ADC corrected low", output_voltage_v[0],
                   0.0f, 2.0e-7f);
    Test_CheckNear(summary, "ADC corrected high", output_voltage_v[1],
                   3.3f, 1.0e-6f);
}

static void Test_ChannelDelay(test_summary_t *summary)
{
    signal_channel_delay_calibration_t calibration;
    float corrected_phase_deg;

    Test_CheckU32(summary, "Channel delay compute status",
        (unsigned int)SignalChannelDelayCalibration_Compute(
            -36.0f, 0.0f, 10000.0f, &calibration),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Channel delay seconds",
                   calibration.delay_b_relative_to_a_s,
                   10.0e-6f, 1.0e-10f);
    Test_CheckU32(summary, "Channel delay apply status",
        (unsigned int)SignalChannelDelayCalibration_Apply(
            -72.0f, 20000.0f, &calibration, &corrected_phase_deg),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Channel corrected phase", corrected_phase_deg,
                   0.0f, 2.0e-5f);
}

static void Test_RobustMeasurements(test_summary_t *summary)
{
    const float vpp_samples_v[5] = {100.0f, 0.0f, 3.0f, 1.0f, 2.0f};
    const signal_robust_peak_to_peak_config_t vpp_config = {0.25f, 0.75f};
    signal_robust_peak_to_peak_result_t vpp_result;
    float vpp_workspace[5];
    const float rms_samples_v[7] = {-1.0f, -1.0f, -1.0f, 0.0f,
                                    1.0f, 1.0f, 100.0f};
    const signal_robust_rms_config_t rms_config = {
        1.0f / 6.0f, 5.0f / 6.0f, 1U};
    signal_robust_rms_result_t rms_result;
    float rms_workspace[7];

    Test_CheckU32(summary, "Robust Vpp status",
        (unsigned int)SignalRobustPeakToPeak_Process(
            vpp_samples_v, 5U, &vpp_config,
            vpp_workspace, 5U, &vpp_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Robust Vpp lower", vpp_result.lower_voltage_v,
                   1.0f, 1.0e-6f);
    Test_CheckNear(summary, "Robust Vpp upper", vpp_result.upper_voltage_v,
                   3.0f, 1.0e-6f);
    Test_CheckNear(summary, "Robust Vpp value", vpp_result.robust_vpp_v,
                   2.0f, 1.0e-6f);

    Test_CheckU32(summary, "Robust RMS status",
        (unsigned int)SignalRobustRMS_Process(
            rms_samples_v, 7U, &rms_config,
            rms_workspace, 7U, &rms_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Robust RMS lower", rms_result.lower_limit_v,
                   -1.0f, 1.0e-6f);
    Test_CheckNear(summary, "Robust RMS upper", rms_result.upper_limit_v,
                   1.0f, 1.0e-6f);
    Test_CheckNear(summary, "Robust RMS mean", rms_result.winsorized_mean_v,
                   0.0f, 1.0e-6f);
    Test_CheckNear(summary, "Robust RMS value", rms_result.robust_rms_v,
                   sqrtf(6.0f / 7.0f), 1.0e-6f);
    Test_CheckU32(summary, "Robust RMS clipped count",
                  (unsigned int)rms_result.winsorized_count, 1U);
}

static void GenerateCosine(
    float *samples,
    uint32_t count,
    float sample_rate_hz,
    float frequency_hz,
    float amplitude_peak_v,
    float phase_deg,
    float dc_offset_v)
{
    uint32_t index;
    float phase_rad = phase_deg * TEST_PI_F / 180.0f;
    for (index = 0U; index < count; ++index)
    {
        float angle = 2.0f * TEST_PI_F * frequency_hz * (float)index /
                      sample_rate_hz;
        samples[index] = dc_offset_v + amplitude_peak_v *
                         cosf(angle + phase_rad);
    }
}

static void Test_SineFit(test_summary_t *summary)
{
    float samples[1000];
    const signal_sine_fit_3param_config_t config3 = {1234.5f, 100000.0f};
    signal_sine_fit_3param_result_t result3;
    const signal_sine_fit_4param_config_t config4 = {
        1220.0f, 80.0f, 100000.0f, 28U};
    signal_sine_fit_4param_result_t result4;

    GenerateCosine(samples, 1000U, 100000.0f, 1234.5f,
                   0.4f, -20.0f, 1.2f);
    Test_CheckU32(summary, "Sine fit 3-param status",
        (unsigned int)SignalSineFit3Param_Process(
            samples, 1000U, &config3, &result3),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Sine fit 3-param amplitude",
                   result3.amplitude_peak_v, 0.4f, 3.0e-5f);
    Test_CheckNear(summary, "Sine fit 3-param phase",
                   result3.phase_deg, -20.0f, 3.0e-3f);
    Test_CheckNear(summary, "Sine fit 3-param DC",
                   result3.dc_offset_v, 1.2f, 3.0e-5f);
    Test_CheckNear(summary, "Sine fit 3-param residual",
                   result3.residual_rms_v, 0.0f, 2.0e-5f);

    Test_CheckU32(summary, "Sine fit 4-param status",
        (unsigned int)SignalSineFit4Param_Process(
            samples, 1000U, &config4, &result4),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Sine fit 4-param frequency",
                   result4.frequency_hz, 1234.5f, 0.08f);
    Test_CheckNear(summary, "Sine fit 4-param amplitude",
                   result4.waveform.amplitude_peak_v, 0.4f, 3.0e-4f);
    Test_CheckNear(summary, "Sine fit 4-param phase",
                   result4.waveform.phase_deg, -20.0f, 0.3f);
    Test_CheckNear(summary, "Sine fit 4-param DC",
                   result4.waveform.dc_offset_v, 1.2f, 1.0e-4f);
}

static void Test_LockIn(test_summary_t *summary)
{
    float samples[1000];
    const signal_lock_in_config_t config = {
        1000.0f, 100000.0f, 0.0f, 1U};
    signal_lock_in_result_t result;

    GenerateCosine(samples, 1000U, 100000.0f, 1000.0f,
                   0.2f, 30.0f, 1.65f);
    Test_CheckU32(summary, "Lock-in status",
        (unsigned int)SignalLockIn_Process(samples, 1000U, &config, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Lock-in mean", result.mean_voltage_v,
                   1.65f, 2.0e-5f);
    Test_CheckNear(summary, "Lock-in amplitude", result.amplitude_peak_v,
                   0.2f, 3.0e-5f);
    Test_CheckNear(summary, "Lock-in phase", result.phase_deg,
                   30.0f, 5.0e-3f);
}

int main(void)
{
    test_summary_t summary = {0U, 0U};

    puts("=== MSPM0 Signal Algorithm PC Test: Seventh Batch ===");
    Test_ADCCalibration(&summary);
    Test_ChannelDelay(&summary);
    Test_RobustMeasurements(&summary);
    Test_SineFit(&summary);
    Test_LockIn(&summary);
    printf("=== SUMMARY: PASS=%u FAIL=%u ===\n", summary.passed, summary.failed);
    return (summary.failed == 0U) ? 0 : 1;
}
