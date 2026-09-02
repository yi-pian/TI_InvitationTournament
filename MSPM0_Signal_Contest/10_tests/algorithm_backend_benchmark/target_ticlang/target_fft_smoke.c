#include <stdint.h>

#include "signal_fft.h"

#ifndef SIGNAL_TARGET_FFT_COUNT
#define SIGNAL_TARGET_FFT_COUNT 512U
#endif

static float target_input[SIGNAL_TARGET_FFT_COUNT];
static signal_complex_f32_t target_spectrum[SIGNAL_TARGET_FFT_COUNT];
static volatile float target_result_sink;

int main(void)
{
    uint32_t index;
    signal_algorithm_status_t status;

    for (index = 0U; index < SIGNAL_TARGET_FFT_COUNT; ++index)
    {
        target_input[index] = (index & 1U) ? 0.25f : -0.25f;
    }
    status = SignalFFT_ForwardReal(
        target_input,
        target_spectrum,
        SIGNAL_TARGET_FFT_COUNT,
        SIGNAL_TARGET_FFT_COUNT);
    target_result_sink = (status == SIGNAL_ALGORITHM_OK) ?
                         target_spectrum[1U].real : (float)status;
    for (;;)
    {
        __asm(" nop");
    }
}
