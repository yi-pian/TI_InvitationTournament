#include "ti_msp_dl_config.h"
#include "signal_peak_detect.h"

static const float g_magnitude[5] = {0.0f, 1.0f, 4.0f, 2.0f, 0.5f};
volatile signal_peak_detect_result_t g_result;
volatile signal_algorithm_status_t g_status;

int main(void)
{
    signal_peak_detect_result_t result;
    SYSCFG_DL_init();
    g_status = SignalPeakDetect_Process(g_magnitude, 5U, 1U, 4U, &result);
    g_result = result;
    while (1) { __WFI(); }
}
