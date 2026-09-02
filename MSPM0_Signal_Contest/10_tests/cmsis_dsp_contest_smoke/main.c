#include <stdint.h>

#include "arm_const_structs.h"
#include "arm_math.h"
#include "ti_msp_dl_config.h"

#define SMOKE_FFT_N (256U)

static const float32_t g_a[4] = {1.0F, 2.0F, 3.0F, 4.0F};
static const float32_t g_b[4] = {4.0F, 3.0F, 2.0F, 1.0F};
static float32_t g_sum[4];
static q15_t g_fft[2U * SMOKE_FFT_N];
static q15_t g_magnitude[SMOKE_FFT_N];

volatile float32_t g_smoke_rms;
volatile float32_t g_smoke_min;
volatile float32_t g_smoke_max;
volatile uint32_t g_smoke_min_index;
volatile uint32_t g_smoke_max_index;
volatile q15_t g_smoke_fft_peak;
volatile uint32_t g_smoke_fft_peak_index;
volatile uint32_t g_smoke_complete;

int main(void)
{
    float32_t rms;
    float32_t minimum;
    float32_t maximum;
    uint32_t minimum_index;
    uint32_t maximum_index;
    q15_t fft_peak;
    uint32_t fft_peak_index;

    SYSCFG_DL_init();

    arm_add_f32(g_a, g_b, g_sum, 4U);
    arm_rms_f32(g_a, 4U, &rms);
    arm_min_f32(g_a, 4U, &minimum, &minimum_index);
    arm_max_f32(g_a, 4U, &maximum, &maximum_index);

    g_fft[0] = (q15_t) 16384;
    arm_cfft_q15(&arm_cfft_sR_q15_len256, g_fft, 0U, 1U);
    arm_cmplx_mag_q15(g_fft, g_magnitude, SMOKE_FFT_N);
    arm_max_q15(g_magnitude, SMOKE_FFT_N, &fft_peak, &fft_peak_index);

    g_smoke_rms = rms;
    g_smoke_min = minimum;
    g_smoke_max = maximum;
    g_smoke_min_index = minimum_index;
    g_smoke_max_index = maximum_index;
    g_smoke_fft_peak = fft_peak;
    g_smoke_fft_peak_index = fft_peak_index;

    g_smoke_complete = 1U;
    __BKPT(0);
    while (1) {
        __WFI();
    }
}
