#include "signal_dac_dma.h"

#include <stddef.h>

signal_result_t SignalDACDMA_Init(signal_dac_dma_t *module, void *context,
    signal_dac_dma_start_fn start, signal_dac_dma_stop_fn stop)
{
    if ((module == NULL) || (start == NULL) || (stop == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    module->context = context;
    module->start = start;
    module->stop = stop;
    module->state = MODULE_IDLE;
    module->active_count = 0U;
    module->repeat = false;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalDACDMA_Start(signal_dac_dma_t *module,
    const uint16_t *samples, size_t count, bool repeat)
{
    signal_result_t result;
    if ((module == NULL) || (module->start == NULL) || (samples == NULL) ||
        (count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (module->state == MODULE_RUNNING) { return SIGNAL_RESULT_BUSY; }
    result = module->start(module->context, samples, count, repeat);
    if (result == SIGNAL_RESULT_OK) {
        module->active_count = count;
        module->repeat = repeat;
        module->state = MODULE_RUNNING;
    } else { module->state = MODULE_ERROR; }
    return result;
}

signal_result_t SignalDACDMA_Stop(signal_dac_dma_t *module)
{
    signal_result_t result;
    if ((module == NULL) || (module->stop == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    result = module->stop(module->context);
    module->state = (result == SIGNAL_RESULT_OK) ? MODULE_IDLE : MODULE_ERROR;
    return result;
}

signal_module_status_t SignalDACDMA_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
