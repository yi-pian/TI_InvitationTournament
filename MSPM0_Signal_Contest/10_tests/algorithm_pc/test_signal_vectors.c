#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "signal_test_vectors.h"
#include "test_helpers.h"

static void Test_AllVectors(test_summary_t *summary)
{
    const signal_test_sine_config_t clean = {1000.0f, 10.0f, 0.5f, 0.0f, 0.0f};
    const signal_test_sine_config_t with_dc = {1000.0f, 10.0f, 0.5f, 1.65f, 0.0f};
    float values[100];
    float repeat[100];
    uint32_t index;

    Test_CheckU32(summary, "clean_sine status",
        (unsigned int)SignalTestVector_CleanSine(values,100U,&clean),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "clean_sine quarter peak", values[25], 0.5f, 1.0e-6f);

    Test_CheckU32(summary, "sine_with_dc status",
        (unsigned int)SignalTestVector_SineWithDC(values,100U,&with_dc),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "sine_with_dc start", values[0], 1.65f, 1.0e-6f);
    Test_CheckNear(summary, "sine_with_dc peak", values[25], 2.15f, 1.0e-6f);

    Test_CheckU32(summary, "noisy_sine status",
        (unsigned int)SignalTestVector_NoisySine(values,100U,&clean,0.1f,123U),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckU32(summary, "noisy_sine repeat status",
        (unsigned int)SignalTestVector_NoisySine(repeat,100U,&clean,0.1f,123U),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    for (index = 0U; index < 100U; ++index)
    {
        if ((values[index] != repeat[index]) ||
            (fabsf(values[index] - 0.5f * sinf(0.02f * 3.14159265358979323846f *
                                               (float)index)) > 0.100001f))
        {
            break;
        }
    }
    Test_CheckU32(summary, "noisy_sine deterministic/bounded", index, 100U);

    Test_CheckU32(summary, "sine_with_harmonics status",
        (unsigned int)SignalTestVector_SineWithHarmonics(
            values,100U,&clean,0.1f,0.05f),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "harmonics quarter sample", values[25],
                   0.45f, 2.0e-6f);

    Test_CheckU32(summary, "square_wave status",
        (unsigned int)SignalTestVector_SquareWave(values,100U,&clean),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "square_wave high", values[25], 0.5f, 1.0e-6f);
    Test_CheckNear(summary, "square_wave low", values[75], -0.5f, 1.0e-6f);

    Test_CheckU32(summary, "triangle_wave status",
        (unsigned int)SignalTestVector_TriangleWave(values,100U,&clean),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "triangle_wave peak", values[25], 0.5f, 1.0e-6f);
    Test_CheckNear(summary, "triangle_wave valley", values[75], -0.5f, 1.0e-6f);

    Test_CheckU32(summary, "impulse_noise status",
        (unsigned int)SignalTestVector_ImpulseNoise(
            values,100U,&clean,25U,10.0f),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "impulse_noise value", values[25], 10.5f, 1.0e-6f);

    Test_CheckU32(summary, "two_tone status",
        (unsigned int)SignalTestVector_TwoTone(
            values,100U,&clean,20.0f,0.2f,0.0f),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "two_tone quarter sample", values[25], 0.5f, 2.0e-6f);

    Test_CheckU32(summary, "burst status",
        (unsigned int)SignalTestVector_Burst(values,100U,&clean,20U,40U),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "burst outside", values[10], 0.0f, 1.0e-6f);
    Test_CheckNear(summary, "burst inside", values[25], 0.5f, 1.0e-6f);

    Test_CheckU32(summary, "clipped_sine status",
        (unsigned int)SignalTestVector_ClippedSine(
            values,100U,&clean,-0.3f,0.3f),
        (unsigned int)SIGNAL_ALGORITHM_OK);
    Test_CheckNear(summary, "clipped_sine upper", values[25], 0.3f, 1.0e-6f);
    Test_CheckNear(summary, "clipped_sine lower", values[75], -0.3f, 1.0e-6f);
}

int main(void)
{
    test_summary_t summary = {0U,0U};
    puts("=== MSPM0 Signal Algorithm PC Test: Signal Vectors ===");
    Test_AllVectors(&summary);
    printf("=== SUMMARY: PASS=%u FAIL=%u ===\n",summary.passed,summary.failed);
    return (summary.failed == 0U) ? 0 : 1;
}
