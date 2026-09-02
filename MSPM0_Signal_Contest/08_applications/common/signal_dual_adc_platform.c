#include "signal_dual_adc_platform.h"

#include <stddef.h>

#include "ti_msp_dl_config.h"

#define SIGNAL_ADC_A_DMA_MASK \
    (DL_DMA_INTERRUPT_CHANNEL0 << SIGNAL_ADC_A_DMA_CHAN_ID)
#define SIGNAL_ADC_B_DMA_MASK \
    (DL_DMA_INTERRUPT_CHANNEL0 << SIGNAL_ADC_B_DMA_CHAN_ID)

static volatile bool s_done_a;
static volatile bool s_done_b;
static bool s_initialized;
static uint32_t s_configured_rate_hz;

signal_result_t SignalDualADCPlatform_Init(uint32_t sample_rate_hz,
    uint32_t timer_clock_hz)
{
    uint32_t timer_count;
    if ((sample_rate_hz == 0U) || (timer_clock_hz == 0U) ||
        (sample_rate_hz > timer_clock_hz)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    timer_count = (timer_clock_hz + sample_rate_hz / 2U) / sample_rate_hz;
    if ((timer_count == 0U) || (timer_count > 65536U)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    DL_TimerG_stopCounter(SIGNAL_DUAL_ADC_TIMER_INST);
    DL_TimerG_setLoadValue(SIGNAL_DUAL_ADC_TIMER_INST, timer_count - 1U);
    DL_TimerG_setTimerCount(SIGNAL_DUAL_ADC_TIMER_INST, timer_count - 1U);
    s_configured_rate_hz = timer_clock_hz / timer_count;
    s_done_a = false;
    s_done_b = false;
    s_initialized = true;
    NVIC_ClearPendingIRQ(SIGNAL_ADC_A_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(SIGNAL_ADC_B_INST_INT_IRQN);
    NVIC_EnableIRQ(SIGNAL_ADC_A_INST_INT_IRQN);
    NVIC_EnableIRQ(SIGNAL_ADC_B_INST_INT_IRQN);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalDualADCPlatform_Start(uint16_t *channel_a,
    uint16_t *channel_b, uint16_t sample_count)
{
    if (!s_initialized) return SIGNAL_RESULT_NOT_INITIALIZED;
    if ((channel_a == NULL) || (channel_b == NULL) || (sample_count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (!(s_done_a && s_done_b) &&
        (DL_TimerG_getTimerCount(SIGNAL_DUAL_ADC_TIMER_INST) !=
         DL_TimerG_getLoadValue(SIGNAL_DUAL_ADC_TIMER_INST))) {
        return SIGNAL_RESULT_BUSY;
    }
    DL_TimerG_stopCounter(SIGNAL_DUAL_ADC_TIMER_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_A_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_B_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_A_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_B_INST);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_A_DMA_CHAN_ID);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_B_DMA_CHAN_ID);
    DL_DMA_clearInterruptStatus(DMA, SIGNAL_ADC_A_DMA_MASK);
    DL_DMA_clearInterruptStatus(DMA, SIGNAL_ADC_B_DMA_MASK);

    DL_DMA_setSrcAddr(DMA, SIGNAL_ADC_A_DMA_CHAN_ID,
        (uint32_t) DL_ADC12_getMemResultAddress(
            SIGNAL_ADC_A_INST, SIGNAL_ADC_A_ADCMEM_0));
    DL_DMA_setDestAddr(
        DMA, SIGNAL_ADC_A_DMA_CHAN_ID, (uint32_t) channel_a);
    DL_DMA_setTransferSize(DMA, SIGNAL_ADC_A_DMA_CHAN_ID, sample_count);
    DL_DMA_setSrcAddr(DMA, SIGNAL_ADC_B_DMA_CHAN_ID,
        (uint32_t) DL_ADC12_getMemResultAddress(
            SIGNAL_ADC_B_INST, SIGNAL_ADC_B_ADCMEM_0));
    DL_DMA_setDestAddr(
        DMA, SIGNAL_ADC_B_DMA_CHAN_ID, (uint32_t) channel_b);
    DL_DMA_setTransferSize(DMA, SIGNAL_ADC_B_DMA_CHAN_ID, sample_count);

    DL_ADC12_clearInterruptStatus(
        SIGNAL_ADC_A_INST, DL_ADC12_INTERRUPT_DMA_DONE);
    DL_ADC12_clearInterruptStatus(
        SIGNAL_ADC_B_INST, DL_ADC12_INTERRUPT_DMA_DONE);
    s_done_a = false;
    s_done_b = false;
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

bool SignalDualADCPlatform_IsFinished(void)
{
    return s_done_a && s_done_b;
}

void SignalDualADCPlatform_Stop(void)
{
    DL_TimerG_stopCounter(SIGNAL_DUAL_ADC_TIMER_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_A_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_B_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_A_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_B_INST);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_A_DMA_CHAN_ID);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_B_DMA_CHAN_ID);
}

uint32_t SignalDualADCPlatform_GetConfiguredRate(void)
{
    return s_configured_rate_hz;
}

void SIGNAL_ADC_A_INST_IRQHandler(void)
{
    if (DL_ADC12_getPendingInterrupt(SIGNAL_ADC_A_INST) ==
        DL_ADC12_IIDX_DMA_DONE) {
        DL_ADC12_disableDMA(SIGNAL_ADC_A_INST);
        DL_ADC12_disableConversions(SIGNAL_ADC_A_INST);
        DL_DMA_disableChannel(DMA, SIGNAL_ADC_A_DMA_CHAN_ID);
        s_done_a = true;
        if (s_done_b) DL_TimerG_stopCounter(SIGNAL_DUAL_ADC_TIMER_INST);
    }
}

void SIGNAL_ADC_B_INST_IRQHandler(void)
{
    if (DL_ADC12_getPendingInterrupt(SIGNAL_ADC_B_INST) ==
        DL_ADC12_IIDX_DMA_DONE) {
        DL_ADC12_disableDMA(SIGNAL_ADC_B_INST);
        DL_ADC12_disableConversions(SIGNAL_ADC_B_INST);
        DL_DMA_disableChannel(DMA, SIGNAL_ADC_B_DMA_CHAN_ID);
        s_done_b = true;
        if (s_done_a) DL_TimerG_stopCounter(SIGNAL_DUAL_ADC_TIMER_INST);
    }
}
