#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "signal_multi_cycle_average.h"
#include "signal_remove_dc.h"
#include "signal_zero_cross.h"
#include "signal_zero_cross_interpolation.h"
#include "test_helpers.h"

#define TEST_PI_F 3.14159265358979323846f
#define TEST_SAMPLE_COUNT 4096U
#define TEST_EVENT_CAPACITY 80U

static void Test_LinearInterpolation(test_summary_t *summary)
{
    const float voltage_v[] = {-1.0f, 3.0f};
    const signal_zero_cross_event_t event = {
        0U, 1U, SIGNAL_ZERO_CROSS_RISING
    };
    float position_samples = 0.0f;
    signal_zero_cross_interpolation_result_t result;

    Test_CheckU32(summary, "Interpolation status",
        (unsigned int)SignalZeroCrossInterpolation_Process(
            voltage_v, 2U, 0.0f, &event, 1U,
            &position_samples, 1U, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Interpolation position", position_samples,
                   0.25f, 1.0e-7f);
    Test_CheckU32(summary, "Interpolation count", result.position_count, 1U);
}

static void Test_MultiCycleKnownPositions(test_summary_t *summary)
{
    const float positions_samples[] = {0.25f, 100.25f, 200.25f, 300.25f};
    signal_multi_cycle_average_result_t result;

    Test_CheckU32(summary, "MultiCycle status",
        (unsigned int)SignalMultiCycleAverage_Process(
            positions_samples, 4U, 100000.0f, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "MultiCycle period samples",
                   result.average_period_samples, 100.0f, 1.0e-6f);
    Test_CheckNear(summary, "MultiCycle frequency",
                   result.frequency_hz, 1000.0f, 1.0e-5f);
    Test_CheckNear(summary, "MultiCycle observation time",
                   result.observation_time_s, 0.003f, 1.0e-8f);
    Test_CheckU32(summary, "MultiCycle cycle count", result.cycle_count, 3U);
}

static void Test_SineWithDC(test_summary_t *summary)
{
    float voltage_v[TEST_SAMPLE_COUNT];
    signal_zero_cross_event_t events[TEST_EVENT_CAPACITY];
    float positions_samples[TEST_EVENT_CAPACITY];
    signal_zero_cross_result_t crossing_result;
    signal_zero_cross_interpolation_result_t interpolation_result;
    signal_multi_cycle_average_result_t frequency_result;
    const signal_zero_cross_config_t config = {
        1.65f, 0.02f, SIGNAL_ZERO_CROSS_RISING
    };
    const float sample_rate_hz = 100000.0f;
    const float expected_frequency_hz = 1234.5f;
    uint32_t index;

    for (index = 0U; index < TEST_SAMPLE_COUNT; ++index)
    {
        voltage_v[index] = 1.65f + 0.5f * sinf(
            2.0f * TEST_PI_F * expected_frequency_hz *
            (float)index / sample_rate_hz - 0.7f);
    }

    Test_CheckU32(summary, "ZeroCross DC status",
        (unsigned int)SignalZeroCross_Process(
            voltage_v, TEST_SAMPLE_COUNT, &config, events,
            TEST_EVENT_CAPACITY, &crossing_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "ZeroCross falling count",
                  crossing_result.falling_count, 0U);

    Test_CheckU32(summary, "Interpolation sine status",
        (unsigned int)SignalZeroCrossInterpolation_Process(
            voltage_v, TEST_SAMPLE_COUNT, config.threshold_v,
            events, crossing_result.event_count,
            positions_samples, TEST_EVENT_CAPACITY,
            &interpolation_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);

    Test_CheckU32(summary, "Frequency status",
        (unsigned int)SignalMultiCycleAverage_Process(
            positions_samples, interpolation_result.position_count,
            sample_rate_hz, &frequency_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Frequency sine with DC",
                   frequency_result.frequency_hz,
                   expected_frequency_hz, 0.01f);
}

static void Test_RemoveDCPath(test_summary_t *summary)
{
    float voltage_v[1000];
    float centered_v[1000];
    signal_zero_cross_event_t events[16];
    float positions_samples[16];
    signal_remove_dc_result_t remove_result;
    signal_zero_cross_result_t crossing_result;
    signal_zero_cross_interpolation_result_t interpolation_result;
    signal_multi_cycle_average_result_t frequency_result;
    const signal_zero_cross_config_t config = {
        0.0f, 0.01f, SIGNAL_ZERO_CROSS_RISING
    };
    uint32_t index;

    for (index = 0U; index < 1000U; ++index)
    {
        voltage_v[index] = 1.65f + 0.5f * sinf(
            2.0f * TEST_PI_F * 1000.0f * (float)index / 100000.0f - 0.4f);
    }

    Test_CheckU32(summary, "RemoveDC path status",
        (unsigned int)SignalRemoveDC_Process(
            voltage_v, centered_v, 1000U, &remove_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "ZeroCross centered status",
        (unsigned int)SignalZeroCross_Process(
            centered_v, 1000U, &config, events, 16U, &crossing_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "Interpolate centered status",
        (unsigned int)SignalZeroCrossInterpolation_Process(
            centered_v, 1000U, 0.0f, events, crossing_result.event_count,
            positions_samples, 16U, &interpolation_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "Centered frequency status",
        (unsigned int)SignalMultiCycleAverage_Process(
            positions_samples, interpolation_result.position_count,
            100000.0f, &frequency_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "Frequency after RemoveDC",
                   frequency_result.frequency_hz, 1000.0f, 0.01f);
}

static void Test_HysteresisAndErrors(test_summary_t *summary)
{
    const float jitter_v[] = {
        -0.20f, -0.01f, 0.01f, -0.01f, 0.01f, 0.20f,
         0.01f, -0.01f, -0.20f, -0.01f, 0.01f, 0.20f
    };
    const signal_zero_cross_config_t config = {
        0.0f, 0.10f, SIGNAL_ZERO_CROSS_RISING
    };
    signal_zero_cross_event_t events[4];
    signal_zero_cross_result_t result;
    const float invalid_positions[] = {10.0f, 9.0f};
    signal_multi_cycle_average_result_t frequency_result;

    Test_CheckU32(summary, "Hysteresis status",
        (unsigned int)SignalZeroCross_Process(
            jitter_v, 12U, &config, events, 4U, &result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "Hysteresis event count", result.event_count, 2U);
    Test_CheckU32(summary, "MultiCycle rejects order",
        (unsigned int)SignalMultiCycleAverage_Process(
            invalid_positions, 2U, 100000.0f, &frequency_result),
        (unsigned int)SIGNAL_ALGORITHM_OUT_OF_RANGE);
}

int main(void)
{
    test_summary_t summary = {0U, 0U};

    puts("=== MSPM0 Signal Algorithm PC Test: Second Batch ===");
    puts("Truth A: Fs=100000 Hz, f=1234.5 Hz, peak=0.5 V, DC=1.65 V");
    puts("Truth B: Fs=100000 Hz, f=1000 Hz, RemoveDC then threshold=0 V");

    Test_LinearInterpolation(&summary);
    Test_MultiCycleKnownPositions(&summary);
    Test_SineWithDC(&summary);
    Test_RemoveDCPath(&summary);
    Test_HysteresisAndErrors(&summary);

    printf("=== SUMMARY: PASS=%u FAIL=%u ===\n", summary.passed, summary.failed);
    return (summary.failed == 0U) ? 0 : 1;
}
