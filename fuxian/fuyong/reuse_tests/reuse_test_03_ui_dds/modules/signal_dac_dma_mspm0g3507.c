/**
 * @file signal_dac_dma_mspm0g3507.c
 * @brief MSPM0G3507 contest edition DAC DMA output.
 * @note Original path: MSPM0_Signal_Contest/06_generator/dac_dma/
 */

#include "signal_dac_dma_mspm0g3507.h"

#include <limits.h>
#include <stddef.h>

#include "ti_msp_dl_config.h"

static volatile signal_status_t s_status = MODULE_IDLE;
static bool s_initialized;
static bool s_repeat;
static uint32_t s_timer_clock_hz;
static uint32_t s_timer_max_count;
static uint32_t s_configured_rate_hz;

static signal_result_t SignalDACDMA_MSPM0_CalculateTimerCount(
    uint32_t update_rate_hz, uint32_t *timer_count, uint32_t *actual_rate_hz)
{
    uint32_t count;
    if ((update_rate_hz == 0U) || (timer_count == NULL) ||
        (actual_rate_hz == NULL) || (s_timer_clock_hz == 0U) ||
        (s_timer_max_count == 0U) || (update_rate_hz > s_timer_clock_hz)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    count = (s_timer_clock_hz + update_rate_hz / 2U) / update_rate_hz;
    if ((count == 0U) || (count > s_timer_max_count)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    *timer_count = count;
    *actual_rate_hz = s_timer_clock_hz / count;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalDACDMA_MSPM0_Init(
    const signal_dac_dma_mspm0_config_t *config)
{
    signal_result_t result;
    if ((config == NULL) || (config->timer_clock_hz == 0U) ||
        (config->timer_max_count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    DL_TimerG_stopCounter(SIGNAL_DAC_TIMER_INST);
    DL_DAC12_disableDMATrigger(DAC0);
    DL_DMA_disableChannel(DMA, SIGNAL_DAC_DMA_CHAN_ID);
    s_timer_clock_hz = config->timer_clock_hz;
    s_timer_max_count = config->timer_max_count;
    s_repeat = false;
    s_initialized = true;
    s_status = MODULE_IDLE;
    NVIC_ClearPendingIRQ(DAC12_INT_IRQN);
    NVIC_EnableIRQ(DAC12_INT_IRQN);
    result = SignalDACDMA_MSPM0_SetUpdateRate(config->update_rate_hz);
    if (result != SIGNAL_RESULT_OK) {
        s_initialized = false;
        s_status = MODULE_ERROR;
    }
    return result;
}

signal_result_t SignalDACDMA_MSPM0_SetUpdateRate(uint32_t update_rate_hz)
{
    uint32_t timer_count;
    uint32_t actual_rate_hz;
    signal_result_t result;
    if (!s_initialized) return SIGNAL_RESULT_NOT_INITIALIZED;
    if (s_status == MODULE_RUNNING) return SIGNAL_RESULT_BUSY;
    result = SignalDACDMA_MSPM0_CalculateTimerCount(
        update_rate_hz, &timer_count, &actual_rate_hz);
    if (result != SIGNAL_RESULT_OK) return result;
    DL_TimerG_stopCounter(SIGNAL_DAC_TIMER_INST);
    DL_TimerG_setLoadValue(SIGNAL_DAC_TIMER_INST, timer_count - 1U);
    DL_TimerG_setTimerCount(SIGNAL_DAC_TIMER_INST, timer_count - 1U);
    s_configured_rate_hz = actual_rate_hz;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalDACDMA_MSPM0_Start(
    const uint16_t *samples, size_t count, bool repeat)
{
    if (!s_initialized) return SIGNAL_RESULT_NOT_INITIALIZED;
    if (s_status == MODULE_RUNNING) return SIGNAL_RESULT_BUSY;
    if ((samples == NULL) || (count == 0U) || (count > UINT16_MAX)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    DL_TimerG_stopCounter(SIGNAL_DAC_TIMER_INST);
    DL_DAC12_disableDMATrigger(DAC0);
    DL_DMA_disableChannel(DMA, SIGNAL_DAC_DMA_CHAN_ID);
    DL_DMA_setSrcAddr(DMA, SIGNAL_DAC_DMA_CHAN_ID, (uint32_t) samples);
    DL_DMA_setDestAddr(
        DMA, SIGNAL_DAC_DMA_CHAN_ID, (uint32_t) &(DAC0->DATA0));
    DL_DMA_setTransferSize(
        DMA, SIGNAL_DAC_DMA_CHAN_ID, (uint16_t) count);
    s_repeat = repeat;
    s_status = MODULE_RUNNING;
    DL_DMA_enableChannel(DMA, SIGNAL_DAC_DMA_CHAN_ID);
    DL_DAC12_enable(DAC0);
    DL_DAC12_enableDMATrigger(DAC0);
    DL_TimerG_setTimerCount(SIGNAL_DAC_TIMER_INST,
        DL_TimerG_getLoadValue(SIGNAL_DAC_TIMER_INST));
    DL_TimerG_startCounter(SIGNAL_DAC_TIMER_INST);
    return SIGNAL_RESULT_OK;
}

void SignalDACDMA_MSPM0_Stop(void)
{
    DL_TimerG_stopCounter(SIGNAL_DAC_TIMER_INST);
    DL_DAC12_disableDMATrigger(DAC0);
    DL_DMA_disableChannel(DMA, SIGNAL_DAC_DMA_CHAN_ID);
    if (s_initialized) s_status = MODULE_IDLE;
}

bool SignalDACDMA_MSPM0_IsFinished(void) { return s_status == MODULE_DONE; }
signal_status_t SignalDACDMA_MSPM0_GetStatus(void) { return s_status; }
uint32_t SignalDACDMA_MSPM0_GetConfiguredRate(void)
{
    return s_configured_rate_hz;
}
signal_module_status_t SignalDACDMA_MSPM0_GetModuleMaturity(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}

void DAC12_IRQHandler(void)
{
    if (DL_DAC12_getPendingInterrupt(DAC0) == DL_DAC12_IIDX_DMA_DONE) {
        if (s_repeat) {
            DL_DAC12_enableDMATrigger(DAC0);
        } else {
            DL_TimerG_stopCounter(SIGNAL_DAC_TIMER_INST);
            DL_DAC12_disableDMATrigger(DAC0);
            DL_DMA_disableChannel(DMA, SIGNAL_DAC_DMA_CHAN_ID);
            s_status = MODULE_DONE;
        }
    }
}
