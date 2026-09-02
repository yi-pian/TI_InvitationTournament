#include "ti_msp_dl_config.h"
#include "signal_remove_dc.h"

static const float g_input[4] = {1.0f, 2.0f, 1.0f, 2.0f};
static float g_output[4];
volatile signal_remove_dc_result_t g_result;
volatile signal_algorithm_status_t g_status;

int main(void)
{
    signal_remove_dc_result_t result;
    SYSCFG_DL_init();
    g_status = SignalRemoveDC_Process(g_input, g_output, 4U, &result);
    g_result = result;
    while (1) { __WFI(); }
}
