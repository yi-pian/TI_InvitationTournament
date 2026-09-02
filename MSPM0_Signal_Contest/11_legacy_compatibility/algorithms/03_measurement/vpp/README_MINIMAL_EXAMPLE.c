#include "ti_msp_dl_config.h"
#include "signal_vpp.h"

static const float g_voltage[4] = {0.5f, 2.5f, 1.0f, 2.0f};
volatile signal_vpp_result_t g_result;
volatile signal_algorithm_status_t g_status;

int main(void)
{
    signal_vpp_result_t result;
    SYSCFG_DL_init();
    g_status = SignalVPP_Process(g_voltage, 4U, &result);
    g_result = result;
    while (1) { __WFI(); }
}
