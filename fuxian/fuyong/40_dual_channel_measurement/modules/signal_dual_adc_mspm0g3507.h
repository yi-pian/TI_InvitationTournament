#ifndef SIGNAL_DUAL_ADC_MSPM0G3507_H
#define SIGNAL_DUAL_ADC_MSPM0G3507_H

/**
 * @file signal_dual_adc_mspm0g3507.h
 * @brief MSPM0G3507 contest edition: Timer/Event/ADC0/ADC1/DMA dual capture.
 * ADC0 is the conditioned waveform channel; ADC1 is the comparator marker
 * channel used by the burst controller and is not an amplitude measurement.
 * @note Original path: MSPM0_Signal_Contest/02_acquisition/adc_dual_sync/
 */

#include <stdbool.h>
#include <stdint.h>

#include "signal_status.h"

typedef struct {
    uint32_t sample_rate_hz;
    uint32_t timer_clock_hz;
    uint32_t timer_max_count;
} signal_dual_adc_config_t;

signal_result_t SignalDualADC_Init(const signal_dual_adc_config_t *config);
signal_result_t SignalDualADC_SetSampleRate(uint32_t sample_rate_hz);
signal_result_t SignalDualADC_Start(uint16_t *channel_a,
    uint16_t *channel_b, uint16_t sample_count);

/*
 * Start the burst acquisition profile. The two ADCs keep sampling from the
 * same timer event while the DMA channels rotate through block_count buffers.
 * channel_a/channel_b point to the first element of contiguous blocks. The
 * DMA IRQ publishes a block only after both channels complete it.
 */
signal_result_t SignalDualADC_StartContinuous(uint16_t *channel_a,
    uint16_t *channel_b, uint16_t samples_per_block, uint8_t block_count);
void SignalDualADC_Stop(void);
bool SignalDualADC_IsFinished(void);
bool SignalDualADC_IsContinuous(void);
uint32_t SignalDualADC_GetContinuousBlockSequence(void);
uint8_t SignalDualADC_GetContinuousCompletedBlock(void);
bool SignalDualADC_GetContinuousSnapshot(uint32_t *sequence,
    uint8_t *completed_block);
signal_status_t SignalDualADC_GetStatus(void);
const uint16_t *SignalDualADC_GetChannelA(void);
const uint16_t *SignalDualADC_GetChannelB(void);
uint16_t SignalDualADC_GetSampleCount(void);
uint32_t SignalDualADC_GetConfiguredRate(void);
signal_module_status_t SignalDualADC_GetModuleMaturity(void);

#endif /* SIGNAL_DUAL_ADC_MSPM0G3507_H */
