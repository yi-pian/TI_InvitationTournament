#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_dac_dma_mspm0g3507.h"

#define SIGNAL_DAC_UPDATE_RATE_HZ  (100000U)

static const uint16_t g_wave[] = {512U, 2048U, 3584U, 2048U};
volatile signal_result_t g_status;

int main(void)
{
    const signal_dac_dma_mspm0_config_t config = {
        SIGNAL_DAC_UPDATE_RATE_HZ, CPUCLK_FREQ, 65536U
    };
    SYSCFG_DL_init();
    g_status = SignalDACDMA_MSPM0_Init(&config);
    if (g_status != SIGNAL_RESULT_OK) while (1) { }
    g_status = SignalDACDMA_MSPM0_Start(
        g_wave, sizeof(g_wave) / sizeof(g_wave[0]), true);
    if (g_status != SIGNAL_RESULT_OK) while (1) { }

    while (1) {
        /* ===== 这里写你自己的逻辑；DMA 正在循环输出 ===== */
        __WFI();
    }
}
