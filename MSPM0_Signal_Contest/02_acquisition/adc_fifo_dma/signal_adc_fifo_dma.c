/**
 * @file signal_adc_fifo_dma.c
 * @brief MSPM0G3507 ADC FIFO + 32-bit DMA 满吞吐率单帧采集实现。
 */

#include "signal_adc_fifo_dma.h"

#include <stddef.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"

/** DMA 完成状态位由 SysConfig 生成的通道号推导。 */
#define SIGNAL_ADC_FIFO_DMA_INTERRUPT_MASK \
    (DL_DMA_INTERRUPT_CHANNEL0 << SIGNAL_ADC_FIFO_DMA_CHAN_ID)

static volatile signal_status_t s_status = MODULE_IDLE;
static uint16_t *s_buffer = NULL;
static uint16_t s_sample_count = 0U;
static uint32_t s_nominal_sample_rate_hz = 0U;
static bool s_initialized = false;

/**
 * @brief 复位 ADC 并重新应用该 SysConfig instance 的 FIFO 配置。
 * @note 这样重复 Start 时不会把上一帧结束后残留的 FIFO 数据带到下一帧。
 */
static void SignalADCFIFODMA_ResetADC(void)
{
    DL_ADC12_disableConversions(SIGNAL_ADC_FIFO_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_FIFO_INST);
    DL_ADC12_reset(SIGNAL_ADC_FIFO_INST);
    DL_ADC12_enablePower(SIGNAL_ADC_FIFO_INST);
    delay_cycles(POWER_STARTUP_DELAY);
    SYSCFG_DL_SIGNAL_ADC_FIFO_init();
}

signal_result_t SignalADCFIFODMA_Init(
    const signal_adc_fifo_dma_config_t *config)
{
    if ((config == NULL) || (config->nominal_sample_rate_hz == 0U)) {
        s_status = MODULE_ERROR;
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    DL_ADC12_disableConversions(SIGNAL_ADC_FIFO_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_FIFO_INST);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_FIFO_DMA_CHAN_ID);

    s_buffer = NULL;
    s_sample_count = 0U;
    s_nominal_sample_rate_hz = config->nominal_sample_rate_hz;
    s_initialized = true;
    s_status = MODULE_IDLE;

    NVIC_ClearPendingIRQ(SIGNAL_ADC_FIFO_INST_INT_IRQN);
    NVIC_EnableIRQ(SIGNAL_ADC_FIFO_INST_INT_IRQN);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalADCFIFODMA_Start(
    uint16_t *buffer, uint16_t sample_count)
{
    if (!s_initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    if (s_status == MODULE_RUNNING) {
        return SIGNAL_RESULT_BUSY;
    }
    if ((buffer == NULL) || (sample_count == 0U) ||
        ((sample_count & 1U) != 0U) ||
        ((((uintptr_t) buffer) & 3U) != 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    DL_DMA_disableChannel(DMA, SIGNAL_ADC_FIFO_DMA_CHAN_ID);
    DL_DMA_clearInterruptStatus(
        DMA, SIGNAL_ADC_FIFO_DMA_INTERRUPT_MASK);
    SignalADCFIFODMA_ResetADC();

    DL_DMA_setSrcAddr(DMA, SIGNAL_ADC_FIFO_DMA_CHAN_ID,
        (uint32_t) DL_ADC12_getFIFOAddress(SIGNAL_ADC_FIFO_INST));
    DL_DMA_setDestAddr(
        DMA, SIGNAL_ADC_FIFO_DMA_CHAN_ID, (uint32_t) &buffer[0]);
    DL_DMA_setTransferSize(
        DMA, SIGNAL_ADC_FIFO_DMA_CHAN_ID, sample_count / 2U);

    DL_ADC12_clearInterruptStatus(
        SIGNAL_ADC_FIFO_INST, DL_ADC12_INTERRUPT_DMA_DONE);
    NVIC_ClearPendingIRQ(SIGNAL_ADC_FIFO_INST_INT_IRQN);

    s_buffer = buffer;
    s_sample_count = sample_count;
    s_status = MODULE_RUNNING;

    DL_DMA_enableChannel(DMA, SIGNAL_ADC_FIFO_DMA_CHAN_ID);
    DL_ADC12_enableDMA(SIGNAL_ADC_FIFO_INST);
    DL_ADC12_enableConversions(SIGNAL_ADC_FIFO_INST);
    DL_ADC12_startConversion(SIGNAL_ADC_FIFO_INST);
    return SIGNAL_RESULT_OK;
}

void SignalADCFIFODMA_Stop(void)
{
    if (!s_initialized) {
        return;
    }

    DL_ADC12_disableConversions(SIGNAL_ADC_FIFO_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_FIFO_INST);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_FIFO_DMA_CHAN_ID);
    s_status = MODULE_IDLE;
}

bool SignalADCFIFODMA_IsFinished(void)
{
    return (s_status == MODULE_DONE);
}

signal_status_t SignalADCFIFODMA_GetStatus(void)
{
    return s_status;
}

const uint16_t *SignalADCFIFODMA_GetBuffer(void)
{
    return s_buffer;
}

uint16_t SignalADCFIFODMA_GetSampleCount(void)
{
    return s_sample_count;
}

uint32_t SignalADCFIFODMA_GetNominalSampleRateHz(void)
{
    return s_nominal_sample_rate_hz;
}

signal_module_status_t SignalADCFIFODMA_GetModuleMaturity(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}

/** ADC DMA 完成中断：停止转换后只发布整帧完成状态。 */
void SIGNAL_ADC_FIFO_INST_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(SIGNAL_ADC_FIFO_INST)) {
        case DL_ADC12_IIDX_DMA_DONE:
            DL_ADC12_disableConversions(SIGNAL_ADC_FIFO_INST);
            DL_ADC12_disableDMA(SIGNAL_ADC_FIFO_INST);
            DL_DMA_disableChannel(DMA, SIGNAL_ADC_FIFO_DMA_CHAN_ID);
            s_status = MODULE_DONE;
            break;
        default:
            break;
    }
}
