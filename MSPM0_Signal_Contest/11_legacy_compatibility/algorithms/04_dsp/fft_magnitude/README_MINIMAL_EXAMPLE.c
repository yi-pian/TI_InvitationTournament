#include "ti_msp_dl_config.h"
#include "signal_fft_magnitude.h"

static const signal_complex_f32_t g_spectrum[8] = {
    {0, 0}, {1, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {1, 0}
};
static float g_magnitude[5];
volatile signal_fft_magnitude_result_t g_result;
volatile signal_algorithm_status_t g_status;

int main(void)
{
    signal_fft_magnitude_result_t result;
    SYSCFG_DL_init();
    g_status = SignalFFTMagnitude_Process(
        g_spectrum, 8U, g_magnitude, 5U, &result);
    g_result = result;
    while (1) { __WFI(); }
}
