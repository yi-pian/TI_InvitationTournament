#include <stdint.h>
#include <stdbool.h>

#include "ti_msp_dl_config.h"
#include "signal_timer_capture_mspm0g3507.h"

#define SIGNAL_CAPTURE_TIMER_HZ  (CPUCLK_FREQ)

static signal_timer_capture_mspm0_result_t g_capture_result;
static signal_result_t g_status;

int main(void)
{
    /* Trigger Capture 使用当前计数器的 LOAD 值；模块内部按 LOAD+1 处理回绕。 */
    const signal_timer_capture_mspm0_config_t config = {
        SIGNAL_CAPTURE_TIMER_HZ, SIGNAL_CAPTURE_INST_LOAD_VALUE
    };
    SYSCFG_DL_init();
    g_status = SignalTimerCapture_MSPM0_Init(&config);
    if (g_status != SIGNAL_RESULT_OK) while (1) { }
    (void)SignalTimerCapture_MSPM0_Start();

    while (1) {
        /* Start 只调用一次；GetResult 在收到两个同向边沿后才会返回 OK。 */
        g_status = SignalTimerCapture_MSPM0_GetResult(&g_capture_result);

        /* ===== 这里写你自己的逻辑：使用 g_capture_result.frequency_hz ===== */
        if (g_status == SIGNAL_RESULT_OK && g_capture_result.valid) {
            /* 显示 g_capture_result.frequency_hz 和 duty_percent。 */
        }
        __WFI();
    }
}
