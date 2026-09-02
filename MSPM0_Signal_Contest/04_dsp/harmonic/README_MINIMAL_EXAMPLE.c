/* 最小示例：从频谱幅值计算指定次数的谐波结果。 */
#include "ti_msp_dl_config.h"
#include "signal_harmonic.h"

static const float g_magnitude[33] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0
};
volatile signal_harmonic_result_t g_result;
volatile signal_algorithm_status_t g_status;

int main(void)
{
    const signal_harmonic_config_t config = {1000.0f, 1U, 3U, 0U};
    signal_harmonic_result_t result;
    SYSCFG_DL_init();
    g_status = SignalHarmonic_Process(
        g_magnitude, 33U, 6400.0f, 64U, &config, &result);
    g_result = result;
    while (1) { __WFI(); }
}
