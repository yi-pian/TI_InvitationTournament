#include "ti_msp_dl_config.h"
#include "signal_ac_rms.h"

static const float g_voltage[4] = {2.0f, 0.0f, 2.0f, 0.0f};
volatile signal_ac_rms_result_t g_result;
volatile signal_algorithm_status_t g_status;

int main(void)
{
    signal_ac_rms_result_t result;
    SYSCFG_DL_init();
    g_status = SignalACRMS_Process(g_voltage, 4U, &result);
    g_result = result;
    while (1) { __WFI(); }
}
