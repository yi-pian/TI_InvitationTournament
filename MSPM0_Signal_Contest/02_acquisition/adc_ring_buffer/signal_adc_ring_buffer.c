#include "signal_adc_ring_buffer.h"

#include <stddef.h>

signal_result_t SignalADCRing_Init(signal_adc_ring_buffer_t *ring,
    uint16_t *storage, size_t capacity)
{
    if ((ring == NULL) || (storage == NULL) || (capacity < 2U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    ring->storage = storage;
    ring->capacity = capacity;
    ring->head = 0U;
    ring->tail = 0U;
    ring->overruns = 0U;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalADCRing_Push(signal_adc_ring_buffer_t *ring,
    uint16_t sample)
{
    size_t next;
    if ((ring == NULL) || (ring->storage == NULL) || (ring->capacity < 2U)) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    next = ring->head + 1U;
    if (next == ring->capacity) {
        next = 0U;
    }
    if (next == ring->tail) {
        ring->overruns++;
        return SIGNAL_RESULT_INSUFFICIENT_BUFFER;
    }
    ring->storage[ring->head] = sample;
    ring->head = next;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalADCRing_Pop(signal_adc_ring_buffer_t *ring,
    uint16_t *sample)
{
    size_t next;
    if ((ring == NULL) || (sample == NULL) || (ring->storage == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (ring->tail == ring->head) {
        return SIGNAL_RESULT_NO_DATA;
    }
    *sample = ring->storage[ring->tail];
    next = ring->tail + 1U;
    ring->tail = (next == ring->capacity) ? 0U : next;
    return SIGNAL_RESULT_OK;
}

size_t SignalADCRing_Count(const signal_adc_ring_buffer_t *ring)
{
    if ((ring == NULL) || (ring->capacity == 0U)) {
        return 0U;
    }
    return (ring->head >= ring->tail) ? (ring->head - ring->tail) :
        (ring->capacity - ring->tail + ring->head);
}

void SignalADCRing_Clear(signal_adc_ring_buffer_t *ring)
{
    if (ring != NULL) {
        ring->head = 0U;
        ring->tail = 0U;
        ring->overruns = 0U;
    }
}

signal_module_status_t SignalADCRing_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
