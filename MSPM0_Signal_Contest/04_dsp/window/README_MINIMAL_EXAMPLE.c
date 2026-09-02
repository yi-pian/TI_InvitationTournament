/* 最小示例：对一帧输入数据应用窗口函数。 */
#include "ti_msp_dl_config.h"
#include "signal_window.h"

static const float g_input[8] = {1, 1, 1, 1, 1, 1, 1, 1};
static float g_output[8];
volatile signal_window_result_t g_result;
volatile signal_algorithm_status_t g_status;

int main(void)
{
    signal_window_result_t result;
    SYSCFG_DL_init();
    g_status = SignalWindow_Apply(
        g_input, g_output, 8U, SIGNAL_WINDOW_HANN, &result);
    g_result = result;
    while (1) { __WFI(); }
}
