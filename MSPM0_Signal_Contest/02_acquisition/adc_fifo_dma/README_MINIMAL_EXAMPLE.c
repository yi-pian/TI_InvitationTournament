#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_adc_fifo_dma.h"

#define SIGNAL_NOMINAL_SAMPLE_RATE_HZ (4000000U)
#define SIGNAL_SAMPLE_COUNT            (1024U)

_Alignas(4) static uint16_t g_raw[SIGNAL_SAMPLE_COUNT];
volatile signal_result_t g_status;

int main(void)
{
    const signal_adc_fifo_dma_config_t config = {
        SIGNAL_NOMINAL_SAMPLE_RATE_HZ
    };

    SYSCFG_DL_init();
    g_status = SignalADCFIFODMA_Init(&config);
    if (g_status != SIGNAL_RESULT_OK) while (1) { }

    while (1) {
        g_status = SignalADCFIFODMA_Start(g_raw, SIGNAL_SAMPLE_COUNT);
        if (g_status != SIGNAL_RESULT_OK) while (1) { }
        while (!SignalADCFIFODMA_IsFinished()) { __WFI(); }

        /* ===== 这里写你自己的逻辑：处理 g_raw[0..N-1] ===== */
    }
}
