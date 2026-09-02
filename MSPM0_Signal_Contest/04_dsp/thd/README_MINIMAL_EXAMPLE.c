/* 最小示例：从基波和谐波能量计算 THD。 */
#include "ti_msp_dl_config.h"
#include "signal_thd.h"

volatile signal_thd_result_t g_result;
volatile signal_algorithm_status_t g_status;

int main(void)
{
    signal_harmonic_result_t harmonics = {0};
    signal_thd_result_t result;
    harmonics.first_order = 1U;
    harmonics.last_order = 3U;
    harmonics.items[1].energy = 100.0f;
    harmonics.items[2].energy = 4.0f;
    harmonics.items[3].energy = 1.0f;
    SYSCFG_DL_init();
    g_status = SignalTHD_Process(&harmonics, &result);
    g_result = result;
    while (1) { __WFI(); }
}
