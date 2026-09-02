#include <stdint.h>
#include <stdio.h>

#include "signal_fir.h"
#include "signal_hampel.h"
#include "signal_iir_biquad.h"
#include "signal_mad.h"
#include "signal_median_filter.h"
#include "signal_moving_average.h"
#include "test_helpers.h"

static void Test_MovingAverage(test_summary_t *summary)
{
    const float input[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    const float expected[] = {1.0f, 1.5f, 2.0f, 3.0f, 4.0f};
    float output[5];
    uint32_t index;

    Test_CheckU32(summary, "MovingAverage status",
        (unsigned int)SignalMovingAverage_Process(input, output, 5U, 3U),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    for (index = 0U; index < 5U; ++index)
    {
        char name[48];
        (void)snprintf(name, sizeof(name), "MovingAverage y[%u]", index);
        Test_CheckNear(summary, name, output[index], expected[index], 1.0e-6f);
    }
}

static void Test_MedianMADHampel(test_summary_t *summary)
{
    const float median_input[] = {1.0f, 1.0f, 100.0f, 1.0f, 1.0f};
    float median_output[5];
    float median_workspace[3];
    const float mad_input[] = {1.0f, 1.0f, 2.0f, 2.0f, 100.0f};
    float mad_workspace[5];
    signal_mad_result_t mad_result;
    const float hampel_input[] = {1.0f, 1.0f, 1.0f, 100.0f, 1.0f, 1.0f, 1.0f};
    float hampel_output[7];
    float hampel_workspace[5];
    const signal_hampel_config_t hampel_config = {5U, 3.0f, 0.0f};
    signal_hampel_result_t hampel_result;
    uint32_t index;

    Test_CheckU32(summary, "Median status",
        (unsigned int)SignalMedianFilter_Process(
            median_input, median_output, 5U, 3U,
            median_workspace, 3U),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    for (index = 0U; index < 5U; ++index)
    {
        Test_CheckNear(summary, "Median removes impulse",
                       median_output[index], 1.0f, 1.0e-6f);
    }

    Test_CheckU32(summary, "MAD status",
        (unsigned int)SignalMAD_Process(
            mad_input, 5U, mad_workspace, 5U, &mad_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "MAD median", mad_result.median_value, 2.0f, 1.0e-6f);
    Test_CheckNear(summary, "MAD value", mad_result.mad_value, 1.0f, 1.0e-6f);
    Test_CheckNear(summary, "MAD robust sigma",
                   mad_result.robust_sigma_estimate, 1.4826f, 1.0e-6f);

    Test_CheckU32(summary, "Hampel status",
        (unsigned int)SignalHampel_Process(
            hampel_input, hampel_output, 7U, &hampel_config,
            hampel_workspace, 5U, &hampel_result),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "Hampel replaced count", hampel_result.replaced_count, 1U);
    for (index = 0U; index < 7U; ++index)
    {
        Test_CheckNear(summary, "Hampel output", hampel_output[index], 1.0f, 1.0e-6f);
    }
}

static void Test_FIR(test_summary_t *summary)
{
    const float coefficients[] = {0.25f, 0.5f, 0.25f};
    float delay_line[3];
    signal_fir_t fir;
    float block_a[] = {1.0f, 0.0f};
    float block_b[] = {0.0f, 0.0f};

    Test_CheckU32(summary, "FIR init status",
        (unsigned int)SignalFIR_Init(&fir, coefficients, 3U, delay_line, 3U),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "FIR block A status",
        (unsigned int)SignalFIR_Process(&fir, block_a, block_a, 2U),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "FIR block B status",
        (unsigned int)SignalFIR_Process(&fir, block_b, block_b, 2U),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "FIR impulse y0", block_a[0], 0.25f, 1.0e-6f);
    Test_CheckNear(summary, "FIR impulse y1", block_a[1], 0.50f, 1.0e-6f);
    Test_CheckNear(summary, "FIR impulse y2", block_b[0], 0.25f, 1.0e-6f);
    Test_CheckNear(summary, "FIR impulse y3", block_b[1], 0.00f, 1.0e-6f);
}

static void Test_IIRBiquad(test_summary_t *summary)
{
    /* 这节等价于 y[n]=0.5*x[n]+0.5*x[n-1]，便于手算。 */
    const signal_iir_biquad_coefficients_t coefficients[] = {
        {0.5f, 0.5f, 0.0f, 0.0f, 0.0f}
    };
    signal_iir_biquad_state_t states[1];
    signal_iir_biquad_t iir;
    float impulse[] = {1.0f, 0.0f, 0.0f, 0.0f};

    Test_CheckU32(summary, "IIR init status",
        (unsigned int)SignalIIRBiquad_Init(
            &iir, coefficients, 1U, states, 1U),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "IIR process status",
        (unsigned int)SignalIIRBiquad_Process(
            &iir, impulse, impulse, 4U),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "IIR impulse y0", impulse[0], 0.5f, 1.0e-6f);
    Test_CheckNear(summary, "IIR impulse y1", impulse[1], 0.5f, 1.0e-6f);
    Test_CheckNear(summary, "IIR impulse y2", impulse[2], 0.0f, 1.0e-6f);
    Test_CheckNear(summary, "IIR impulse y3", impulse[3], 0.0f, 1.0e-6f);
}

static void Test_ParameterErrors(test_summary_t *summary)
{
    const float input[] = {1.0f, 2.0f, 3.0f};
    float output[3];
    float workspace[2];

    Test_CheckU32(summary, "MovingAverage rejects in-place",
        (unsigned int)SignalMovingAverage_Process(input, (float *)input, 3U, 2U),
        (unsigned int)SIGNAL_ALGORITHM_INVALID_ARGUMENT);
    Test_CheckU32(summary, "Median rejects even window",
        (unsigned int)SignalMedianFilter_Process(
            input, output, 3U, 2U, workspace, 2U),
        (unsigned int)SIGNAL_ALGORITHM_INSUFFICIENT_DATA);
}

int main(void)
{
    test_summary_t summary = {0U, 0U};

    puts("=== MSPM0 Signal Algorithm PC Test: Third Batch ===");
    puts("Truth: short hand-calculated sequences and impulse responses");
    Test_MovingAverage(&summary);
    Test_MedianMADHampel(&summary);
    Test_FIR(&summary);
    Test_IIRBiquad(&summary);
    Test_ParameterErrors(&summary);
    printf("=== SUMMARY: PASS=%u FAIL=%u ===\n", summary.passed, summary.failed);
    return (summary.failed == 0U) ? 0 : 1;
}
