#include "signal_adc_pingpong_dma.h"

#include <stddef.h>

signal_result_t SignalADCPingPong_Init(signal_adc_pingpong_dma_t *module,
    uint16_t *buffer_a, uint16_t *buffer_b, size_t sample_count)
{
    if ((module == NULL) || (buffer_a == NULL) || (buffer_b == NULL) ||
        (buffer_a == buffer_b) || (sample_count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    module->buffer[0] = buffer_a;
    module->buffer[1] = buffer_b;
    module->sample_count = sample_count;
    module->ready[0] = false;
    module->ready[1] = false;
    module->dma_target = SIGNAL_PINGPONG_BUFFER_A;
    module->completed_blocks = 0U;
    module->overrun_blocks = 0U;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalADCPingPong_OnDmaComplete(signal_adc_pingpong_dma_t *module,
    uint16_t **next_destination)
{
    signal_pingpong_buffer_id_t completed;
    signal_pingpong_buffer_id_t next;
    if ((module == NULL) || (next_destination == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    completed = module->dma_target;
    next = (completed == SIGNAL_PINGPONG_BUFFER_A) ?
        SIGNAL_PINGPONG_BUFFER_B : SIGNAL_PINGPONG_BUFFER_A;
    if (module->ready[completed]) {
        module->overrun_blocks++;
    }
    module->ready[completed] = true;
    module->dma_target = next;
    module->completed_blocks++;
    *next_destination = module->buffer[next];
    return module->ready[next] ? SIGNAL_RESULT_BUSY : SIGNAL_RESULT_OK;
}

signal_result_t SignalADCPingPong_Acquire(signal_adc_pingpong_dma_t *module,
    signal_pingpong_buffer_id_t *id, const uint16_t **samples, size_t *count)
{
    signal_pingpong_buffer_id_t candidate;
    if ((module == NULL) || (id == NULL) || (samples == NULL) ||
        (count == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    candidate = (module->dma_target == SIGNAL_PINGPONG_BUFFER_A) ?
        SIGNAL_PINGPONG_BUFFER_B : SIGNAL_PINGPONG_BUFFER_A;
    if (!module->ready[candidate]) {
        candidate = module->dma_target;
    }
    if (!module->ready[candidate]) {
        return SIGNAL_RESULT_NO_DATA;
    }
    *id = candidate;
    *samples = module->buffer[candidate];
    *count = module->sample_count;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalADCPingPong_Release(signal_adc_pingpong_dma_t *module,
    signal_pingpong_buffer_id_t id)
{
    if ((module == NULL) || (id > SIGNAL_PINGPONG_BUFFER_B)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    module->ready[id] = false;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalADCPingPong_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
