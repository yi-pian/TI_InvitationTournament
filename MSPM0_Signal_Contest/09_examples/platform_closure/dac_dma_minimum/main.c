#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_dac_dma.h"
#include "signal_dac_dma_platform.h"

static const uint16_t g_wave_table[] = {
    2048U, 3495U, 4095U, 3495U, 2048U, 600U, 0U, 600U
};
volatile int32_t g_dac_dma_status;

int main(void)
{
    signal_dac_dma_t dac_dma;

    SYSCFG_DL_init();
    g_dac_dma_status = (int32_t) SignalDACPlatform_Init(8000U, CPUCLK_FREQ);
    if (g_dac_dma_status == (int32_t) SIGNAL_RESULT_OK) {
        g_dac_dma_status = (int32_t) SignalDACDMA_Init(&dac_dma, NULL,
            SignalDACPlatform_Start, SignalDACPlatform_Stop);
    }
    if (g_dac_dma_status == (int32_t) SIGNAL_RESULT_OK) {
        g_dac_dma_status = (int32_t) SignalDACDMA_Start(&dac_dma,
            g_wave_table, sizeof(g_wave_table) / sizeof(g_wave_table[0]),
            true);
    }
    while (1) __WFI();
}
