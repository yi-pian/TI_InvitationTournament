/* 最小示例：使用两路同步波形计算相位差。 */
#include "ti_msp_dl_config.h"
#include "signal_phase.h"

volatile signal_phase_result_t g_result;
volatile signal_algorithm_status_t g_status;

int main(void)
{
    signal_phase_result_t result;
    SYSCFG_DL_init();
    g_status = SignalPhase_FromZeroCross(10.0f, 12.0f, 8.0f, &result);
    g_result = result;
    while (1) { __WFI(); }
}
