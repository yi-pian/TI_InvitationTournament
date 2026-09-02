#include "signal_dma.h"

#include <stddef.h>

signal_result_t SignalDMA_ValidateTransfer(const signal_dma_transfer_t *transfer)
{
    if ((transfer == NULL) || (transfer->source == (uintptr_t) 0U) ||
        (transfer->destination == (uintptr_t) 0U) ||
        (transfer->transfer_count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if ((transfer->width != SIGNAL_DMA_WIDTH_8) &&
        (transfer->width != SIGNAL_DMA_WIDTH_16) &&
        (transfer->width != SIGNAL_DMA_WIDTH_32)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    if (((transfer->source % (uintptr_t) transfer->width) != 0U) ||
        ((transfer->destination % (uintptr_t) transfer->width) != 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalDMA_Start(const signal_dma_t *dma,
    const signal_dma_transfer_t *transfer)
{
    signal_result_t result = SignalDMA_ValidateTransfer(transfer);
    if (result != SIGNAL_RESULT_OK) {
        return result;
    }
    if ((dma == NULL) || (dma->configure == NULL) || (dma->start == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    result = dma->configure(dma->context, transfer);
    return (result == SIGNAL_RESULT_OK) ? dma->start(dma->context) : result;
}

signal_result_t SignalDMA_Stop(const signal_dma_t *dma)
{
    if ((dma == NULL) || (dma->stop == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return dma->stop(dma->context);
}

signal_module_status_t SignalDMA_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
