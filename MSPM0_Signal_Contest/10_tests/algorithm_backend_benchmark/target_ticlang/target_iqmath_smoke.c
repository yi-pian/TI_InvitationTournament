#include "signal_phase.h"
#include "signal_rms.h"

static volatile float target_iqmath_sink;

int main(void)
{
    const float samples[4] = {1.0f, -1.0f, 1.0f, -1.0f};
    const signal_complex_f32_t spectrum_a[1] = {{1.0f, 0.0f}};
    const signal_complex_f32_t spectrum_b[1] = {{0.0f, 1.0f}};
    signal_rms_result_t rms_result;
    signal_phase_result_t phase_result;

    (void)SignalRMS_Process(samples, 4U, &rms_result);
    (void)SignalPhase_FromFFTBin(
        spectrum_a, spectrum_b, 1U, 0U, &phase_result);
    target_iqmath_sink = rms_result.rms_v + phase_result.phase_difference_deg;
    for (;;)
    {
        __asm(" nop");
    }
}
