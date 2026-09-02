/**
 * @file signal_dual_adc_mspm0g3507.c
 * @brief MSPM0G3507 contest edition dual ADC DMA capture.
 * @note Original path: MSPM0_Signal_Contest/02_acquisition/adc_dual_sync/
 */

#include "signal_dual_adc_mspm0g3507.h"

#include <stddef.h>

#include "ti_msp_dl_config.h"

#define SIGNAL_DUAL_ADC_A_DMA_MASK \
    (DL_DMA_INTERRUPT_CHANNEL0 << SIGNAL_ADC_A_DMA_CHAN_ID)
#define SIGNAL_DUAL_ADC_B_DMA_MASK \
    (DL_DMA_INTERRUPT_CHANNEL0 << SIGNAL_ADC_B_DMA_CHAN_ID)

static volatile bool s_done_a;
static volatile bool s_done_b;
static volatile signal_status_t s_status = MODULE_IDLE;
static bool s_initialized;
static uint32_t s_timer_clock_hz;
static uint32_t s_timer_max_count;
static uint32_t s_configured_rate_hz;
static uint16_t *s_channel_a;
static uint16_t *s_channel_b;
static uint16_t s_sample_count;
static uint16_t *s_continuous_base_a;
static uint16_t *s_continuous_base_b;
static uint16_t s_continuous_samples_per_block;
static uint8_t s_continuous_block_count;
static volatile uint8_t s_continuous_active_block;
static volatile uint8_t s_continuous_completed_block;
static volatile uint32_t s_continuous_block_sequence;
static volatile bool s_continuous;

static void SignalDualADC_SetDMATransferMode(
    DL_DMA_TRANSFER_MODE transfer_mode)
{
    DL_DMA_setTransferMode(DMA, SIGNAL_ADC_A_DMA_CHAN_ID, transfer_mode);
    DL_DMA_setTransferMode(DMA, SIGNAL_ADC_B_DMA_CHAN_ID, transfer_mode);
}

static void SignalDualADC_ConfigureDMABlock(uint8_t block_index)
{
    uint16_t *destination_a = s_continuous_base_a +
        ((uint32_t)block_index * s_continuous_samples_per_block);
    uint16_t *destination_b = s_continuous_base_b +
        ((uint32_t)block_index * s_continuous_samples_per_block);

    DL_DMA_disableChannel(DMA, SIGNAL_ADC_A_DMA_CHAN_ID);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_B_DMA_CHAN_ID);
    DL_DMA_setSrcAddr(DMA, SIGNAL_ADC_A_DMA_CHAN_ID,
        (uint32_t)DL_ADC12_getMemResultAddress(
            SIGNAL_ADC_A_INST, SIGNAL_ADC_A_ADCMEM_0));
    DL_DMA_setDestAddr(DMA, SIGNAL_ADC_A_DMA_CHAN_ID,
        (uint32_t)destination_a);
    DL_DMA_setTransferSize(DMA, SIGNAL_ADC_A_DMA_CHAN_ID,
        s_continuous_samples_per_block);
    DL_DMA_setSrcAddr(DMA, SIGNAL_ADC_B_DMA_CHAN_ID,
        (uint32_t)DL_ADC12_getMemResultAddress(
            SIGNAL_ADC_B_INST, SIGNAL_ADC_B_ADCMEM_0));
    DL_DMA_setDestAddr(DMA, SIGNAL_ADC_B_DMA_CHAN_ID,
        (uint32_t)destination_b);
    DL_DMA_setTransferSize(DMA, SIGNAL_ADC_B_DMA_CHAN_ID,
        s_continuous_samples_per_block);
    DL_DMA_clearInterruptStatus(DMA, SIGNAL_DUAL_ADC_A_DMA_MASK |
        SIGNAL_DUAL_ADC_B_DMA_MASK);
    DL_DMA_enableChannel(DMA, SIGNAL_ADC_A_DMA_CHAN_ID);
    DL_DMA_enableChannel(DMA, SIGNAL_ADC_B_DMA_CHAN_ID);
}

static signal_result_t SignalDualADC_CalculateTimerCount(
    uint32_t sample_rate_hz, uint32_t *timer_count, uint32_t *actual_rate_hz)
{
    uint32_t count;
    if ((sample_rate_hz == 0U) || (timer_count == NULL) ||
        (actual_rate_hz == NULL) || (s_timer_clock_hz == 0U) ||
        (s_timer_max_count == 0U) || (sample_rate_hz > s_timer_clock_hz)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    count = (s_timer_clock_hz + sample_rate_hz / 2U) / sample_rate_hz;
    if ((count == 0U) || (count > s_timer_max_count)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    *timer_count = count;
    *actual_rate_hz = s_timer_clock_hz / count;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalDualADC_Init(const signal_dual_adc_config_t *config)
{
    signal_result_t result;
    if ((config == NULL) || (config->timer_clock_hz == 0U) ||
        (config->timer_max_count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    SignalDualADC_Stop();
    s_timer_clock_hz = config->timer_clock_hz;
    s_timer_max_count = config->timer_max_count;
    s_channel_a = NULL;
    s_channel_b = NULL;
    s_sample_count = 0U;
    s_continuous_base_a = NULL;
    s_continuous_base_b = NULL;
    s_continuous_samples_per_block = 0U;
    s_continuous_block_count = 0U;
    s_continuous_active_block = 0U;
    s_continuous_completed_block = UINT8_MAX;
    s_continuous_block_sequence = 0U;
    s_continuous = false;
    s_done_a = false;
    s_done_b = false;
    s_initialized = true;
    s_status = MODULE_IDLE;
    /* Block completion is handled once in DMA_IRQHandler.  The ADC DMA_DONE
     * CPU interrupts are disabled so they cannot race the DMA flags. */
    DL_ADC12_disableInterrupt(SIGNAL_ADC_A_INST,
        DL_ADC12_INTERRUPT_DMA_DONE);
    DL_ADC12_disableInterrupt(SIGNAL_ADC_B_INST,
        DL_ADC12_INTERRUPT_DMA_DONE);
    NVIC_ClearPendingIRQ(DMA_INT_IRQn);
    NVIC_EnableIRQ(DMA_INT_IRQn);
    /* Enable both DMA completion sources here so the module owns its full
     * interrupt setup; applications only need to call SignalDualADC_Init(). */
    DL_DMA_enableInterrupt(DMA, SIGNAL_DUAL_ADC_A_DMA_MASK |
        SIGNAL_DUAL_ADC_B_DMA_MASK);
    result = SignalDualADC_SetSampleRate(config->sample_rate_hz);
    if (result != SIGNAL_RESULT_OK) {
        s_initialized = false;
        s_status = MODULE_ERROR;
    }
    return result;
}

signal_result_t SignalDualADC_SetSampleRate(uint32_t sample_rate_hz)
{
    uint32_t timer_count;
    uint32_t actual_rate_hz;
    signal_result_t result;
    if (!s_initialized) return SIGNAL_RESULT_NOT_INITIALIZED;
    if (s_status == MODULE_RUNNING) return SIGNAL_RESULT_BUSY;
    result = SignalDualADC_CalculateTimerCount(
        sample_rate_hz, &timer_count, &actual_rate_hz);
    if (result != SIGNAL_RESULT_OK) return result;
    DL_TimerG_stopCounter(SIGNAL_DUAL_ADC_TIMER_INST);
    DL_TimerG_setLoadValue(SIGNAL_DUAL_ADC_TIMER_INST, timer_count - 1U);
    DL_TimerG_setTimerCount(SIGNAL_DUAL_ADC_TIMER_INST, timer_count - 1U);
    s_configured_rate_hz = actual_rate_hz;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalDualADC_Start(uint16_t *channel_a,
    uint16_t *channel_b, uint16_t sample_count)
{
    if (!s_initialized) return SIGNAL_RESULT_NOT_INITIALIZED;
    if (s_status == MODULE_RUNNING) return SIGNAL_RESULT_BUSY;
    if ((channel_a == NULL) || (channel_b == NULL) ||
        (sample_count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    DL_TimerG_stopCounter(SIGNAL_DUAL_ADC_TIMER_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_A_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_B_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_A_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_B_INST);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_A_DMA_CHAN_ID);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_B_DMA_CHAN_ID);
    SignalDualADC_SetDMATransferMode(DL_DMA_SINGLE_TRANSFER_MODE);
    DL_DMA_clearInterruptStatus(DMA, SIGNAL_DUAL_ADC_A_DMA_MASK);
    DL_DMA_clearInterruptStatus(DMA, SIGNAL_DUAL_ADC_B_DMA_MASK);

    DL_DMA_setSrcAddr(DMA, SIGNAL_ADC_A_DMA_CHAN_ID,
        (uint32_t) DL_ADC12_getMemResultAddress(
            SIGNAL_ADC_A_INST, SIGNAL_ADC_A_ADCMEM_0));
    DL_DMA_setDestAddr(DMA, SIGNAL_ADC_A_DMA_CHAN_ID, (uint32_t) channel_a);
    DL_DMA_setTransferSize(DMA, SIGNAL_ADC_A_DMA_CHAN_ID, sample_count);
    DL_DMA_setSrcAddr(DMA, SIGNAL_ADC_B_DMA_CHAN_ID,
        (uint32_t) DL_ADC12_getMemResultAddress(
            SIGNAL_ADC_B_INST, SIGNAL_ADC_B_ADCMEM_0));
    DL_DMA_setDestAddr(DMA, SIGNAL_ADC_B_DMA_CHAN_ID, (uint32_t) channel_b);
    DL_DMA_setTransferSize(DMA, SIGNAL_ADC_B_DMA_CHAN_ID, sample_count);

    DL_ADC12_clearInterruptStatus(
        SIGNAL_ADC_A_INST, DL_ADC12_INTERRUPT_DMA_DONE);
    DL_ADC12_clearInterruptStatus(
        SIGNAL_ADC_B_INST, DL_ADC12_INTERRUPT_DMA_DONE);
    NVIC_ClearPendingIRQ(DMA_INT_IRQn);
    s_channel_a = channel_a;
    s_channel_b = channel_b;
    s_sample_count = sample_count;
    s_continuous = false;
    s_continuous_base_a = NULL;
    s_continuous_base_b = NULL;
    s_continuous_block_count = 0U;
    s_continuous_completed_block = UINT8_MAX;
    s_continuous_block_sequence = 0U;
    s_done_a = false;
    s_done_b = false;
    s_status = MODULE_RUNNING;

    DL_DMA_enableChannel(DMA, SIGNAL_ADC_A_DMA_CHAN_ID);
    DL_DMA_enableChannel(DMA, SIGNAL_ADC_B_DMA_CHAN_ID);
    DL_ADC12_enableDMA(SIGNAL_ADC_A_INST);
    DL_ADC12_enableDMA(SIGNAL_ADC_B_INST);
    DL_ADC12_enableConversions(SIGNAL_ADC_A_INST);
    DL_ADC12_enableConversions(SIGNAL_ADC_B_INST);
    DL_TimerG_setTimerCount(SIGNAL_DUAL_ADC_TIMER_INST,
        DL_TimerG_getLoadValue(SIGNAL_DUAL_ADC_TIMER_INST));
    DL_TimerG_startCounter(SIGNAL_DUAL_ADC_TIMER_INST);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalDualADC_StartContinuous(uint16_t *channel_a,
    uint16_t *channel_b, uint16_t samples_per_block, uint8_t block_count)
{
    if (!s_initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    if (s_status == MODULE_RUNNING) {
        return SIGNAL_RESULT_BUSY;
    }
    if ((channel_a == NULL) || (channel_b == NULL) ||
        (samples_per_block == 0U) || (block_count < 2U) ||
        (block_count > 3U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    DL_TimerG_stopCounter(SIGNAL_DUAL_ADC_TIMER_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_A_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_B_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_A_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_B_INST);

    s_channel_a = channel_a;
    s_channel_b = channel_b;
    s_sample_count = samples_per_block;
    s_continuous_base_a = channel_a;
    s_continuous_base_b = channel_b;
    s_continuous_samples_per_block = samples_per_block;
    s_continuous_block_count = block_count;
    s_continuous_active_block = 0U;
    s_continuous_completed_block = UINT8_MAX;
    s_continuous_block_sequence = 0U;
    s_done_a = false;
    s_done_b = false;
    s_continuous = true;
    s_status = MODULE_RUNNING;

    /* FULL-channel repeat mode reloads the transfer count at every block. */
    SignalDualADC_SetDMATransferMode(
        DL_DMA_FULL_CH_REPEAT_SINGLE_TRANSFER_MODE);
    DL_ADC12_clearInterruptStatus(SIGNAL_ADC_A_INST,
        DL_ADC12_INTERRUPT_DMA_DONE);
    DL_ADC12_clearInterruptStatus(SIGNAL_ADC_B_INST,
        DL_ADC12_INTERRUPT_DMA_DONE);
    NVIC_ClearPendingIRQ(DMA_INT_IRQn);
    SignalDualADC_ConfigureDMABlock(0U);
    DL_ADC12_enableDMA(SIGNAL_ADC_A_INST);
    DL_ADC12_enableDMA(SIGNAL_ADC_B_INST);
    DL_ADC12_enableConversions(SIGNAL_ADC_A_INST);
    DL_ADC12_enableConversions(SIGNAL_ADC_B_INST);
    DL_TimerG_setTimerCount(SIGNAL_DUAL_ADC_TIMER_INST,
        DL_TimerG_getLoadValue(SIGNAL_DUAL_ADC_TIMER_INST));
    DL_TimerG_startCounter(SIGNAL_DUAL_ADC_TIMER_INST);
    return SIGNAL_RESULT_OK;
}

void SignalDualADC_Stop(void)
{
    /* Publish the stopped state before touching hardware so a pending DMA IRQ
     * cannot rotate another block after the application requested a stop. */
    s_continuous = false;
    if (s_initialized) {
        s_status = MODULE_IDLE;
    }
    DL_TimerG_stopCounter(SIGNAL_DUAL_ADC_TIMER_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_A_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_B_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_A_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_B_INST);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_A_DMA_CHAN_ID);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_B_DMA_CHAN_ID);
    SignalDualADC_SetDMATransferMode(DL_DMA_SINGLE_TRANSFER_MODE);
    DL_DMA_clearInterruptStatus(DMA, SIGNAL_DUAL_ADC_A_DMA_MASK |
        SIGNAL_DUAL_ADC_B_DMA_MASK);
}

bool SignalDualADC_IsFinished(void) { return s_status == MODULE_DONE; }
bool SignalDualADC_IsContinuous(void) { return s_continuous; }
uint32_t SignalDualADC_GetContinuousBlockSequence(void)
{
    return s_continuous_block_sequence;
}
uint8_t SignalDualADC_GetContinuousCompletedBlock(void)
{
    return s_continuous_completed_block;
}
bool SignalDualADC_GetContinuousSnapshot(uint32_t *sequence,
    uint8_t *completed_block)
{
    uint32_t sequence_before;
    uint32_t sequence_after;

    if ((sequence == NULL) || (completed_block == NULL)) {
        return false;
    }
    do {
        sequence_before = s_continuous_block_sequence;
        *completed_block = s_continuous_completed_block;
        sequence_after = s_continuous_block_sequence;
    } while (sequence_before != sequence_after);
    *sequence = sequence_after;
    return s_continuous && (*completed_block != UINT8_MAX);
}
signal_status_t SignalDualADC_GetStatus(void) { return s_status; }
const uint16_t *SignalDualADC_GetChannelA(void) { return s_channel_a; }
const uint16_t *SignalDualADC_GetChannelB(void) { return s_channel_b; }
uint16_t SignalDualADC_GetSampleCount(void) { return s_sample_count; }
uint32_t SignalDualADC_GetConfiguredRate(void) { return s_configured_rate_hz; }
signal_module_status_t SignalDualADC_GetModuleMaturity(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}

void DMA_IRQHandler(void)
{
    uint32_t pending = DL_DMA_getRawInterruptStatus(DMA,
        SIGNAL_DUAL_ADC_A_DMA_MASK | SIGNAL_DUAL_ADC_B_DMA_MASK);

    if ((pending & SIGNAL_DUAL_ADC_A_DMA_MASK) != 0U) {
        DL_DMA_clearInterruptStatus(DMA, SIGNAL_DUAL_ADC_A_DMA_MASK);
        s_done_a = true;
    }
    if ((pending & SIGNAL_DUAL_ADC_B_DMA_MASK) != 0U) {
        DL_DMA_clearInterruptStatus(DMA, SIGNAL_DUAL_ADC_B_DMA_MASK);
        s_done_b = true;
    }

    if (!(s_done_a && s_done_b)) {
        return;
    }

    if (s_continuous) {
        /* Stop the trigger timer at the exact paired-block boundary.  This
         * bounds the re-arm gap to one ISR, instead of allowing one ADC to
         * overwrite the next block while the other ISR is still pending. */
        DL_TimerG_stopCounter(SIGNAL_DUAL_ADC_TIMER_INST);
        s_continuous_completed_block = s_continuous_active_block;
        s_continuous_block_sequence++;
        s_continuous_active_block = (uint8_t)
            ((s_continuous_active_block + 1U) % s_continuous_block_count);
        s_done_a = false;
        s_done_b = false;
        SignalDualADC_ConfigureDMABlock(s_continuous_active_block);
        DL_ADC12_enableDMA(SIGNAL_ADC_A_INST);
        DL_ADC12_enableDMA(SIGNAL_ADC_B_INST);
        DL_TimerG_setTimerCount(SIGNAL_DUAL_ADC_TIMER_INST,
            DL_TimerG_getLoadValue(SIGNAL_DUAL_ADC_TIMER_INST));
        DL_TimerG_startCounter(SIGNAL_DUAL_ADC_TIMER_INST);
        return;
    }

    DL_TimerG_stopCounter(SIGNAL_DUAL_ADC_TIMER_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_A_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_B_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_A_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_B_INST);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_A_DMA_CHAN_ID);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_B_DMA_CHAN_ID);
    s_status = MODULE_DONE;
}
