#include "signal_dac_dma_platform.h"

#include <limits.h>

#include "ti_msp_dl_config.h"

static volatile bool s_one_shot_finished;
static volatile bool s_repeat;
static bool s_initialized;
static uint32_t s_configured_rate_hz;

signal_result_t SignalDACPlatform_Init(uint32_t update_rate_hz,
    uint32_t timer_clock_hz)
{
    uint32_t timer_count;
    if ((update_rate_hz == 0U) || (timer_clock_hz == 0U) ||
        (update_rate_hz > timer_clock_hz)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    timer_count = (timer_clock_hz + update_rate_hz / 2U) / update_rate_hz;
    if ((timer_count == 0U) || (timer_count > 65536U)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    DL_TimerG_stopCounter(SIGNAL_DAC_TIMER_INST);
    DL_TimerG_setLoadValue(SIGNAL_DAC_TIMER_INST, timer_count - 1U);
    DL_TimerG_setTimerCount(SIGNAL_DAC_TIMER_INST, timer_count - 1U);
    s_configured_rate_hz = timer_clock_hz / timer_count;
    s_one_shot_finished = false;
    s_initialized = true;
    NVIC_ClearPendingIRQ(DAC12_INT_IRQN);
    NVIC_EnableIRQ(DAC12_INT_IRQN);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalDACPlatform_Start(void *context,
    const uint16_t *samples, size_t count, bool repeat)
{
    (void) context;
    if (!s_initialized) return SIGNAL_RESULT_NOT_INITIALIZED;
    if ((samples == NULL) || (count == 0U) || (count > UINT16_MAX)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    DL_TimerG_stopCounter(SIGNAL_DAC_TIMER_INST);
    DL_DAC12_disableDMATrigger(DAC0);
    DL_DMA_disableChannel(DMA, SIGNAL_DAC_DMA_CHAN_ID);
    DL_DMA_setSrcAddr(
        DMA, SIGNAL_DAC_DMA_CHAN_ID, (uint32_t) samples);
    DL_DMA_setDestAddr(
        DMA, SIGNAL_DAC_DMA_CHAN_ID, (uint32_t) &(DAC0->DATA0));
    DL_DMA_setTransferSize(DMA, SIGNAL_DAC_DMA_CHAN_ID, (uint16_t) count);
    s_repeat = repeat;
    s_one_shot_finished = false;
    DL_DMA_enableChannel(DMA, SIGNAL_DAC_DMA_CHAN_ID);
    DL_DAC12_enable(DAC0);
    DL_DAC12_enableDMATrigger(DAC0);
    DL_TimerG_setTimerCount(SIGNAL_DAC_TIMER_INST,
        DL_TimerG_getLoadValue(SIGNAL_DAC_TIMER_INST));
    DL_TimerG_startCounter(SIGNAL_DAC_TIMER_INST);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalDACPlatform_Stop(void *context)
{
    (void) context;
    if (!s_initialized) return SIGNAL_RESULT_NOT_INITIALIZED;
    DL_TimerG_stopCounter(SIGNAL_DAC_TIMER_INST);
    DL_DAC12_disableDMATrigger(DAC0);
    DL_DMA_disableChannel(DMA, SIGNAL_DAC_DMA_CHAN_ID);
    return SIGNAL_RESULT_OK;
}

bool SignalDACPlatform_IsOneShotFinished(void)
{
    return s_one_shot_finished;
}

uint32_t SignalDACPlatform_GetConfiguredRate(void)
{
    return s_configured_rate_hz;
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
            s_one_shot_finished = true;
        }
    }
}
