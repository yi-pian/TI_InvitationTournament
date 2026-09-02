/*
 * ADC DMA 单通道采集测试
 * 验证 Timer->Event->ADC->DMA->RAM 单次块采集流程。
 */

#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "signal_adc_dma.h"

#define SAMPLE_COUNT (64U)
#define TARGET_SAMPLE_RATE_HZ (100000U)

static uint16_t g_raw[SAMPLE_COUNT];
volatile signal_status_t g_adc_dma_status;

int main(void)
{
    const signal_adc_dma_config_t config = {
        .sample_rate_hz = TARGET_SAMPLE_RATE_HZ,
        .timer_clock_hz = CPUCLK_FREQ,
        .timer_max_count = 65536U,
    };

    SYSCFG_DL_init();
    g_adc_dma_status = (signal_status_t) SignalADC_Init(&config);
    
    if (g_adc_dma_status != SIGNAL_RESULT_OK) {
        while (1) __WFI();
    }
    
    g_adc_dma_status = (signal_status_t) SignalADC_Start(g_raw, SAMPLE_COUNT);

    /* 等待采集完成 */
    while ((g_adc_dma_status == (signal_status_t)SIGNAL_RESULT_OK) &&
           !SignalADC_IsFinished()) {
        __WFE();
    }
    
    if (g_adc_dma_status == MODULE_DONE) {
        /* 采集成功，进入空闲循环 */
        while (1) __WFI();
    } else {
        /* 采集失败或发生错误 */
        while (1) __WFI();
    }
}
