#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "benchmark_config.h"
#include "signal_cmsis_dsp_backend.h"
#include "signal_iqmath_backend.h"
#include "signal_reference_backend.h"
#include "signal_types.h"
#include "ti_msp_dl_config.h"

#if ((SIGNAL_BENCHMARK_FFT_SIZE != 256) && \
     (SIGNAL_BENCHMARK_FFT_SIZE != 512) && \
     (SIGNAL_BENCHMARK_FFT_SIZE != 1024) && \
     (SIGNAL_BENCHMARK_FFT_SIZE != 2048) && \
     (SIGNAL_BENCHMARK_FFT_SIZE != 4096))
#error SIGNAL_BENCHMARK_FFT_SIZE must be 256, 512, 1024, 2048 or 4096
#endif

#define SIGNAL_BENCHMARK_Q15_HALF INT16_C(16384)
#define SIGNAL_BENCHMARK_Q24_HALF INT32_C(8388608)
#define SIGNAL_BENCHMARK_Q24_QUARTER INT32_C(4194304)
#define SIGNAL_BENCHMARK_Q24_PI_OVER_4 INT32_C(13176795)
#define SIGNAL_BENCHMARK_Q24_SQRT_HALF INT32_C(11863283)
#define SIGNAL_BENCHMARK_SYSTICK_PERIOD UINT32_C(16777216)

#if SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_REFERENCE_F32
static signal_complex_f32_t g_fft_buffer[SIGNAL_BENCHMARK_FFT_SIZE];
#elif SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_Q15
static int16_t g_fft_buffer[SIGNAL_BENCHMARK_FFT_SIZE * 2U];
static int16_t g_magnitude_buffer[SIGNAL_BENCHMARK_FFT_SIZE];
#elif SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_Q31
static int32_t g_fft_buffer[SIGNAL_BENCHMARK_FFT_SIZE * 2U];
#elif SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_F32
static float g_fft_buffer[SIGNAL_BENCHMARK_FFT_SIZE * 2U];
static float g_magnitude_buffer[SIGNAL_BENCHMARK_FFT_SIZE];
#else
#error Unsupported SIGNAL_BENCHMARK_FFT_BACKEND
#endif

static int16_t g_rms_q15[1024U];

volatile uint32_t g_benchmark_cpu_hz;
volatile uint32_t g_benchmark_fft_size;
volatile uint32_t g_benchmark_fft_backend;
volatile uint64_t g_fft_cycles;
volatile uint64_t g_magnitude_cycles;
volatile uint64_t g_rms_cycles;
volatile uint64_t g_iq_multiply_cycles;
volatile uint64_t g_iq_divide_cycles;
volatile uint64_t g_iq_sqrt_cycles;
volatile uint64_t g_iq_atan2_cycles;
volatile uint64_t g_iq_sincos_cycles;
volatile uint32_t g_iq_multiply_abs_error;
volatile uint32_t g_iq_divide_abs_error;
volatile uint32_t g_iq_sqrt_abs_error;
volatile uint32_t g_iq_atan2_abs_error;
volatile uint32_t g_iq_sin_abs_error;
volatile uint32_t g_iq_cos_abs_error;
volatile bool g_cycles_valid;
volatile bool g_fft_pass;
volatile bool g_iq_pass;
volatile bool g_benchmark_complete;
volatile signal_result_t g_last_result;

volatile int16_t g_rms_result_q15;
volatile signal_iq24_t g_iq_multiply_result;
volatile signal_iq24_t g_iq_divide_result;
volatile signal_iq24_t g_iq_sqrt_result;
volatile signal_iq24_t g_iq_atan2_result;
volatile signal_iq24_t g_iq_sin_result;
volatile signal_iq24_t g_iq_cos_result;

static volatile uint32_t g_systick_wrap_count;

void SysTick_Handler(void)
{
    g_systick_wrap_count++;
}

static void Benchmark_CycleStart(void)
{
    SysTick->CTRL = 0U;
    SysTick->LOAD = SIGNAL_BENCHMARK_SYSTICK_PERIOD - 1U;
    SysTick->VAL = 0U;
    g_systick_wrap_count = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |
        SysTick_CTRL_ENABLE_Msk;
}

static uint64_t Benchmark_CycleStop(void)
{
    uint32_t wraps;
    uint32_t current;
    __disable_irq();
    wraps = g_systick_wrap_count;
    current = SysTick->VAL;
    SysTick->CTRL = 0U;
    __enable_irq();
    return (uint64_t) wraps * SIGNAL_BENCHMARK_SYSTICK_PERIOD +
        (uint64_t) ((SIGNAL_BENCHMARK_SYSTICK_PERIOD - 1U) - current);
}

static uint32_t Benchmark_AbsDifference(signal_iq24_t actual,
    signal_iq24_t expected)
{
    int64_t difference = (int64_t) actual - (int64_t) expected;
    if (difference < 0) { difference = -difference; }
    return (uint32_t) difference;
}

static void Benchmark_PrepareFFT(void)
{
    size_t index;
#if SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_REFERENCE_F32
    for (index = 0U; index < SIGNAL_BENCHMARK_FFT_SIZE; ++index) {
        g_fft_buffer[index].real = 0.0f;
        g_fft_buffer[index].imag = 0.0f;
    }
    g_fft_buffer[0].real = 0.5f;
#elif SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_Q15
    for (index = 0U; index < SIGNAL_BENCHMARK_FFT_SIZE * 2U; ++index) {
        g_fft_buffer[index] = 0;
    }
    g_fft_buffer[0] = SIGNAL_BENCHMARK_Q15_HALF;
#elif SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_Q31
    for (index = 0U; index < SIGNAL_BENCHMARK_FFT_SIZE * 2U; ++index) {
        g_fft_buffer[index] = 0;
    }
    g_fft_buffer[0] = INT32_C(1073741824);
#else
    for (index = 0U; index < SIGNAL_BENCHMARK_FFT_SIZE * 2U; ++index) {
        g_fft_buffer[index] = 0.0f;
    }
    g_fft_buffer[0] = 0.5f;
#endif
}

static signal_result_t Benchmark_RunFFT(void)
{
#if SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_REFERENCE_F32
    return SignalReference_FFTF32(g_fft_buffer, SIGNAL_BENCHMARK_FFT_SIZE,
        false);
#elif SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_Q15
    return SignalCMSISDSP_FFTQ15(g_fft_buffer, SIGNAL_BENCHMARK_FFT_SIZE,
        false);
#elif SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_Q31
    return SignalCMSISDSP_FFTQ31(g_fft_buffer, SIGNAL_BENCHMARK_FFT_SIZE,
        false);
#else
    return SignalCMSISDSP_FFTF32(g_fft_buffer, SIGNAL_BENCHMARK_FFT_SIZE,
        false);
#endif
}

static void Benchmark_RunCMSISAuxiliary(void)
{
    size_t index;
    for (index = 0U; index < 1024U; ++index) {
        g_rms_q15[index] = ((index & 1U) == 0U) ?
            SIGNAL_BENCHMARK_Q15_HALF : -SIGNAL_BENCHMARK_Q15_HALF;
    }
    Benchmark_CycleStart();
    g_last_result = SignalCMSISDSP_RMSQ15(g_rms_q15, 1024U,
        (int16_t *) &g_rms_result_q15);
    g_rms_cycles = Benchmark_CycleStop();

#if SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_Q15
    Benchmark_CycleStart();
    g_last_result = SignalCMSISDSP_MagnitudeQ15(g_fft_buffer,
        SIGNAL_BENCHMARK_FFT_SIZE, g_magnitude_buffer,
        SIGNAL_BENCHMARK_FFT_SIZE);
    g_magnitude_cycles = Benchmark_CycleStop();
#elif SIGNAL_BENCHMARK_FFT_BACKEND == SIGNAL_BENCHMARK_FFT_CMSIS_F32
    Benchmark_CycleStart();
    g_last_result = SignalCMSISDSP_MagnitudeF32(g_fft_buffer,
        SIGNAL_BENCHMARK_FFT_SIZE, g_magnitude_buffer,
        SIGNAL_BENCHMARK_FFT_SIZE);
    g_magnitude_cycles = Benchmark_CycleStop();
#else
    g_magnitude_cycles = 0U;
#endif
}

static void Benchmark_RunIQMath(void)
{
#if SIGNAL_BENCHMARK_ENABLE_IQMATH
    Benchmark_CycleStart();
    g_iq_multiply_result = SignalIQMathQ24_Multiply(
        SIGNAL_BENCHMARK_Q24_HALF, SIGNAL_BENCHMARK_Q24_HALF);
    g_iq_multiply_cycles = Benchmark_CycleStop();

    Benchmark_CycleStart();
    g_last_result = SignalIQMathQ24_Divide(SIGNAL_BENCHMARK_Q24_HALF,
        SIGNAL_BENCHMARK_Q24_QUARTER,
        (signal_iq24_t *) &g_iq_divide_result);
    g_iq_divide_cycles = Benchmark_CycleStop();

    Benchmark_CycleStart();
    g_last_result = SignalIQMathQ24_Sqrt(SIGNAL_BENCHMARK_Q24_QUARTER,
        (signal_iq24_t *) &g_iq_sqrt_result);
    g_iq_sqrt_cycles = Benchmark_CycleStop();

    Benchmark_CycleStart();
    g_iq_atan2_result = SignalIQMathQ24_Atan2(
        SIGNAL_BENCHMARK_Q24_HALF, SIGNAL_BENCHMARK_Q24_HALF);
    g_iq_atan2_cycles = Benchmark_CycleStop();

    Benchmark_CycleStart();
    g_iq_sin_result = SignalIQMathQ24_Sin(
        SIGNAL_BENCHMARK_Q24_PI_OVER_4);
    g_iq_cos_result = SignalIQMathQ24_Cos(
        SIGNAL_BENCHMARK_Q24_PI_OVER_4);
    g_iq_sincos_cycles = Benchmark_CycleStop();

    g_iq_multiply_abs_error = Benchmark_AbsDifference(
        g_iq_multiply_result, SIGNAL_BENCHMARK_Q24_QUARTER);
    g_iq_divide_abs_error = Benchmark_AbsDifference(g_iq_divide_result,
        INT32_C(33554432));
    g_iq_sqrt_abs_error = Benchmark_AbsDifference(g_iq_sqrt_result,
        SIGNAL_BENCHMARK_Q24_HALF);
    g_iq_atan2_abs_error = Benchmark_AbsDifference(g_iq_atan2_result,
        SIGNAL_BENCHMARK_Q24_PI_OVER_4);
    g_iq_sin_abs_error = Benchmark_AbsDifference(g_iq_sin_result,
        SIGNAL_BENCHMARK_Q24_SQRT_HALF);
    g_iq_cos_abs_error = Benchmark_AbsDifference(g_iq_cos_result,
        SIGNAL_BENCHMARK_Q24_SQRT_HALF);
    g_iq_pass =
        (g_iq_multiply_abs_error < 8U) &&
        (g_iq_divide_abs_error < 64U) &&
        (g_iq_sqrt_abs_error < 64U) &&
        (g_iq_atan2_abs_error < 2048U) &&
        (g_iq_sin_abs_error < 2048U) &&
        (g_iq_cos_abs_error < 2048U);
#else
    g_iq_pass = false;
#endif
}

int main(void)
{
    SYSCFG_DL_init();
    g_benchmark_cpu_hz = CPUCLK_FREQ;
    g_benchmark_fft_size = SIGNAL_BENCHMARK_FFT_SIZE;
    g_benchmark_fft_backend = SIGNAL_BENCHMARK_FFT_BACKEND;
    g_benchmark_complete = false;
    g_fft_pass = false;
    g_iq_pass = false;
    g_last_result = SIGNAL_RESULT_OK;

    Benchmark_PrepareFFT();
    Benchmark_CycleStart();
    g_last_result = Benchmark_RunFFT();
    g_fft_cycles = Benchmark_CycleStop();
    g_fft_pass = (g_last_result == SIGNAL_RESULT_OK);
    Benchmark_RunCMSISAuxiliary();
    Benchmark_RunIQMath();
    g_cycles_valid = true;
    g_benchmark_complete = true;
    __BKPT(0);
    while (1) { __WFI(); }
}
