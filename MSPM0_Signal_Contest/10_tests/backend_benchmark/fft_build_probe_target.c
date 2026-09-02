#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "benchmark_config.h"
#include "signal_cmsis_dsp_backend.h"
#include "signal_reference_backend.h"
#include "signal_types.h"
#include "ti_msp_dl_config.h"

#if SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_REFERENCE_F32
static signal_complex_f32_t g_probe_buffer[SIGNAL_BENCHMARK_FFT_SIZE];
#elif SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_Q15
static int16_t g_probe_buffer[SIGNAL_BENCHMARK_FFT_SIZE * 2U];
#elif SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_Q31
static int32_t g_probe_buffer[SIGNAL_BENCHMARK_FFT_SIZE * 2U];
#elif SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_F32
static float g_probe_buffer[SIGNAL_BENCHMARK_FFT_SIZE * 2U];
#else
#error Unsupported SIGNAL_BENCHMARK_FFT_BACKEND
#endif

volatile signal_result_t g_probe_result;
volatile bool g_probe_complete;

int main(void)
{
    SYSCFG_DL_init();
#if SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_REFERENCE_F32
    g_probe_buffer[0].real = 0.5f;
    g_probe_result = SignalReference_FFTF32(g_probe_buffer,
        SIGNAL_BENCHMARK_FFT_SIZE, false);
#elif SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_Q15
    g_probe_buffer[0] = INT16_C(16384);
    g_probe_result = SignalCMSISDSP_FFTQ15(g_probe_buffer,
        SIGNAL_BENCHMARK_FFT_SIZE, false);
#elif SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_Q31
    g_probe_buffer[0] = INT32_C(1073741824);
    g_probe_result = SignalCMSISDSP_FFTQ31(g_probe_buffer,
        SIGNAL_BENCHMARK_FFT_SIZE, false);
#else
    g_probe_buffer[0] = 0.5f;
    g_probe_result = SignalCMSISDSP_FFTF32(g_probe_buffer,
        SIGNAL_BENCHMARK_FFT_SIZE, false);
#endif
    g_probe_complete = true;
    __BKPT(0);
    while (1) { __WFI(); }
}
