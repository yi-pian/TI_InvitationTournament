#ifndef SIGNAL_DAC_DMA_MSPM0G3507_H
#define SIGNAL_DAC_DMA_MSPM0G3507_H

/**
 * @file signal_dac_dma_mspm0g3507.h
 * @brief MSPM0G3507 contest edition: Timer/Event/DMA/DAC continuous output.
 * @note Original path: MSPM0_Signal_Contest/06_generator/dac_dma/
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "signal_status.h"

typedef struct {
    uint32_t update_rate_hz;
    uint32_t timer_clock_hz;
    uint32_t timer_max_count;
} signal_dac_dma_mspm0_config_t;

signal_result_t SignalDACDMA_MSPM0_Init(
    const signal_dac_dma_mspm0_config_t *config);
signal_result_t SignalDACDMA_MSPM0_SetUpdateRate(uint32_t update_rate_hz);
signal_result_t SignalDACDMA_MSPM0_Start(
    const uint16_t *samples, size_t count, bool repeat);
void SignalDACDMA_MSPM0_Stop(void);
bool SignalDACDMA_MSPM0_IsFinished(void);
signal_status_t SignalDACDMA_MSPM0_GetStatus(void);
uint32_t SignalDACDMA_MSPM0_GetConfiguredRate(void);
signal_module_status_t SignalDACDMA_MSPM0_GetModuleMaturity(void);

#endif /* SIGNAL_DAC_DMA_MSPM0G3507_H */
