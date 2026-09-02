#ifndef SIGNAL_DMA_H
#define SIGNAL_DMA_H

#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

typedef enum {
    SIGNAL_DMA_WIDTH_8 = 1,
    SIGNAL_DMA_WIDTH_16 = 2,
    SIGNAL_DMA_WIDTH_32 = 4
} signal_dma_width_t;

typedef struct {
    uintptr_t source;
    uintptr_t destination;
    size_t transfer_count;
    signal_dma_width_t width;
    int source_increment;
    int destination_increment;
} signal_dma_transfer_t;

typedef signal_result_t (*signal_dma_configure_fn)(void *context,
    const signal_dma_transfer_t *transfer);
typedef signal_result_t (*signal_dma_control_fn)(void *context);

typedef struct {
    void *context;
    signal_dma_configure_fn configure;
    signal_dma_control_fn start;
    signal_dma_control_fn stop;
} signal_dma_t;

signal_result_t SignalDMA_ValidateTransfer(const signal_dma_transfer_t *transfer);
signal_result_t SignalDMA_Start(const signal_dma_t *dma,
    const signal_dma_transfer_t *transfer);
signal_result_t SignalDMA_Stop(const signal_dma_t *dma);
signal_module_status_t SignalDMA_GetModuleStatus(void);

#endif
