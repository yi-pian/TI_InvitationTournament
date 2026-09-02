#include "signal_adc_timer_trigger.h"

#include <stddef.h>

signal_result_t SignalADCTimerTrigger_Init(signal_adc_timer_trigger_t *module,
    const signal_timer_t *timer, void *adc_context,
    signal_trigger_control_fn arm_adc, signal_trigger_control_fn disarm_adc,
    uint32_t requested_rate_hz)
{
    signal_result_t result;
    if ((module == NULL) || (timer == NULL) || (arm_adc == NULL) ||
        (disarm_adc == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    module->timer = *timer;
    module->adc_context = adc_context;
    module->arm_adc = arm_adc;
    module->disarm_adc = disarm_adc;
    module->state = MODULE_IDLE;
    result = SignalTimer_SetRate(&module->timer, requested_rate_hz,
        &module->configured_trigger_rate_hz);
    if (result != SIGNAL_RESULT_OK) {
        module->state = MODULE_ERROR;
    }
    return result;
}

signal_result_t SignalADCTimerTrigger_Start(signal_adc_timer_trigger_t *module)
{
    signal_result_t result;
    if ((module == NULL) || (module->arm_adc == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (module->state == MODULE_RUNNING) {
        return SIGNAL_RESULT_BUSY;
    }
    result = module->arm_adc(module->adc_context);
    if (result != SIGNAL_RESULT_OK) {
        module->state = MODULE_ERROR;
        return result;
    }
    result = SignalTimer_Start(&module->timer);
    module->state = (result == SIGNAL_RESULT_OK) ? MODULE_RUNNING : MODULE_ERROR;
    return result;
}

signal_result_t SignalADCTimerTrigger_Stop(signal_adc_timer_trigger_t *module)
{
    signal_result_t timer_result;
    signal_result_t adc_result;
    if ((module == NULL) || (module->disarm_adc == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    timer_result = SignalTimer_Stop(&module->timer);
    adc_result = module->disarm_adc(module->adc_context);
    module->state = ((timer_result == SIGNAL_RESULT_OK) &&
        (adc_result == SIGNAL_RESULT_OK)) ? MODULE_IDLE : MODULE_ERROR;
    return (timer_result != SIGNAL_RESULT_OK) ? timer_result : adc_result;
}

signal_module_status_t SignalADCTimerTrigger_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
