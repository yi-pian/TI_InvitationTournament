/* 最小示例：使用峰值左右三个 bin 做抛物线插值。 */
#include "ti_msp_dl_config.h"
#include "signal_fft_parabolic_interpolation.h"

static const float g_magnitude[5] = {0.0f, 1.0f, 4.0f, 2.0f, 0.5f};
volatile signal_fft_parabolic_result_t g_result;
volatile signal_algorithm_status_t g_status;

int main(void)
{
    signal_fft_parabolic_result_t result;
    SYSCFG_DL_init();
    g_status = SignalFFTParabolicInterpolation_Process(
        g_magnitude, 5U, 2U, 8000.0f, 8U, &result);
    g_result = result;
    while (1) { __WFI(); }
}
