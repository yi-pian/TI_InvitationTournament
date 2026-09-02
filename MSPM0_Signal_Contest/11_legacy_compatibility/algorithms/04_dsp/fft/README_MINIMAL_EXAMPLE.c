#include "ti_msp_dl_config.h"
#include "signal_fft.h"

static const float g_input[8] = {0, 1, 0, -1, 0, 1, 0, -1};
static signal_complex_f32_t g_spectrum[8];
volatile signal_algorithm_status_t g_status;

int main(void)
{
    SYSCFG_DL_init();
    g_status = SignalFFT_ForwardReal(g_input, g_spectrum, 8U, 8U);
    while (1) { __WFI(); }
}
