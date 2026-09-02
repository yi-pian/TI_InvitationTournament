#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_adc_dma.h"

#define SAMPLE_COUNT 64U
#define SAMPLE_RATE_HZ 100000U

static uint16_t g_raw[SAMPLE_COUNT];
volatile int32_t g_adc_dma_status;
volatile uint32_t g_adc_dma_finished;

int main(void)
{
    const signal_adc_dma_config_t config = {
        .sample_rate_hz = SAMPLE_RATE_HZ,
        .timer_clock_hz = CPUCLK_FREQ,
        .timer_max_count = 65536U,
    };

    SYSCFG_DL_init();
    g_adc_dma_status = (int32_t) SignalADC_Init(&config);
    if (g_adc_dma_status == (int32_t) SIGNAL_RESULT_OK) {
        g_adc_dma_status = (int32_t) SignalADC_Start(g_raw, SAMPLE_COUNT);
    }
    while ((g_adc_dma_status == (int32_t) SIGNAL_RESULT_OK) &&
           !SignalADC_IsFinished()) {
        __WFE();
    }
    g_adc_dma_finished = SignalADC_IsFinished() ? 1U : 0U;
    while (1) __WFI();
}
