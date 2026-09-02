#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_adc.h"
#include "signal_adc_timer_trigger.h"
#include "signal_mspm0g3507_platform.h"
#include "signal_timer.h"

volatile int32_t g_adc_trigger_status;
volatile uint32_t g_configured_trigger_rate_hz;

int main(void)
{
    signal_adc_t adc;
    signal_timer_t timer;
    signal_adc_timer_trigger_t trigger;
    signal_mspm0g3507_adc_context_t adc_context = {
        .instance = SIGNAL_ADC_INST,
        .memory_index = SIGNAL_ADC_ADCMEM_0,
        .result_interrupt_mask = DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED,
        .timeout_iterations = 1000000U,
    };

    SYSCFG_DL_init();
    g_adc_trigger_status = (int32_t) SignalMSPM0G3507_ADC_Bind(
        &adc, &adc_context, 2U, 12U,
        SIGNAL_ADC_ADCMEM_0_REF_VOLTAGE_V, CPUCLK_FREQ);
    if (g_adc_trigger_status == (int32_t) SIGNAL_RESULT_OK) {
        g_adc_trigger_status = (int32_t) SignalMSPM0G3507_Timer_Bind(
            &timer, SIGNAL_SAMPLE_TIMER_INST, CPUCLK_FREQ, 65536U);
    }
    if (g_adc_trigger_status == (int32_t) SIGNAL_RESULT_OK) {
        g_adc_trigger_status = (int32_t) SignalADCTimerTrigger_Init(
            &trigger, &timer, &adc_context,
            SignalMSPM0G3507_ADC_Enable,
            SignalMSPM0G3507_ADC_Disable, 100000U);
    }
    if (g_adc_trigger_status == (int32_t) SIGNAL_RESULT_OK) {
        g_configured_trigger_rate_hz = trigger.configured_trigger_rate_hz;
        g_adc_trigger_status = (int32_t) SignalADCTimerTrigger_Start(
            &trigger);
    }
    if (g_adc_trigger_status == (int32_t) SIGNAL_RESULT_OK) {
        g_adc_trigger_status = (int32_t) SignalADCTimerTrigger_Stop(
            &trigger);
    }
    while (1) __WFI();
}
