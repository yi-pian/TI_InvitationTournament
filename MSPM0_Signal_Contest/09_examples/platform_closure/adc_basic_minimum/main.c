#include <stdint.h>

#include "ti_msp_dl_config.h"

volatile uint16_t g_adc_raw;

int main(void)
{
    SYSCFG_DL_init();
    DL_ADC12_clearInterruptStatus(
        SIGNAL_BASIC_ADC_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_startConversion(SIGNAL_BASIC_ADC_INST);
    while (DL_ADC12_getRawInterruptStatus(
               SIGNAL_BASIC_ADC_INST,
               DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0U) {
    }
    g_adc_raw = DL_ADC12_getMemResult(
        SIGNAL_BASIC_ADC_INST, SIGNAL_BASIC_ADC_ADCMEM_0);
    while (1) __WFI();
}
