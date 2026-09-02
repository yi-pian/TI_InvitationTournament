#include "signal_adc_continuous.h"

#include <stddef.h>

signal_result_t SignalADCContinuous_Init(signal_adc_continuous_t *module,
    signal_adc_frame_callback_t callback, void *callback_context)
{
    if ((module == NULL) || (callback == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    module->callback = callback;
    module->callback_context = callback_context;
    module->completed_frames = 0U;
    module->dropped_frames = 0U;
    module->state = MODULE_IDLE;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalADCContinuous_Start(signal_adc_continuous_t *module)
{
    if ((module == NULL) || (module->callback == NULL)) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    if (module->state == MODULE_RUNNING) {
        return SIGNAL_RESULT_BUSY;
    }
    module->state = MODULE_RUNNING;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalADCContinuous_Stop(signal_adc_continuous_t *module)
{
    if (module == NULL) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    module->state = MODULE_IDLE;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalADCContinuous_SubmitFrame(signal_adc_continuous_t *module,
    const uint16_t *samples, size_t count)
{
    if ((module == NULL) || (samples == NULL) || (count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (module->state != MODULE_RUNNING) {
        module->dropped_frames++;
        return SIGNAL_RESULT_BUSY;
    }
    module->completed_frames++;
    module->callback(module->callback_context, samples, count,
        module->completed_frames);
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalADCContinuous_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
