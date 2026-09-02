#include "ti_msp_dl_config.h"
#include "signal_rms.h"

static const float g_voltage[4] = {1.0f, -1.0f, 1.0f, -1.0f};
volatile signal_rms_result_t g_result;
volatile signal_algorithm_status_t g_status;

int main(void)
{
    signal_rms_result_t result;
    SYSCFG_DL_init();
    g_status = SignalRMS_Process(g_voltage, 4U, &result);
    g_result = result;
    while (1) { __WFI(); }
}
