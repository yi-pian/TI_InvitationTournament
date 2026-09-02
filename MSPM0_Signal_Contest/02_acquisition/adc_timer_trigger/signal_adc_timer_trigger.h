#ifndef SIGNAL_ADC_TIMER_TRIGGER_H
#define SIGNAL_ADC_TIMER_TRIGGER_H

#include <stdint.h>
#include "signal_status.h"
#include "signal_timer.h"

typedef signal_result_t (*signal_trigger_control_fn)(void *context);

typedef struct {
    signal_timer_t timer;
    void *adc_context;
    signal_trigger_control_fn arm_adc;
    signal_trigger_control_fn disarm_adc;
    uint32_t configured_trigger_rate_hz;
    signal_status_t state;
} signal_adc_timer_trigger_t;

signal_result_t SignalADCTimerTrigger_Init(signal_adc_timer_trigger_t *module,
    const signal_timer_t *timer, void *adc_context,
    signal_trigger_control_fn arm_adc, signal_trigger_control_fn disarm_adc,
    uint32_t requested_rate_hz);
signal_result_t SignalADCTimerTrigger_Start(signal_adc_timer_trigger_t *module);
signal_result_t SignalADCTimerTrigger_Stop(signal_adc_timer_trigger_t *module);
signal_module_status_t SignalADCTimerTrigger_GetModuleStatus(void);

#endif
