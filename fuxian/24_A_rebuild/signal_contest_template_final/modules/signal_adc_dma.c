/**
 * @file signal_adc_dma.c
 * @brief Timer -> Event -> ADC -> DMA -> RAM 单次块采集实现。
 */

#include "signal_adc_dma.h"

#include <stddef.h>

#include "ti_msp_dl_config.h"

/** DMA 通道完成位按通道号逐位排列；由 SysConfig 生成的通道号推导掩码。 */
#define SIGNAL_ADC_DMA_INTERRUPT_MASK \
    (DL_DMA_INTERRUPT_CHANNEL0 << SIGNAL_ADC_DMA_CHAN_ID)

static volatile signal_status_t s_status = MODULE_IDLE;
static uint16_t *s_buffer = NULL;
static uint16_t s_sample_count = 0U;
static uint32_t s_timer_clock_hz = 0U;
static uint32_t s_timer_max_count = 0U;
static uint32_t s_configured_trigger_rate_hz = 0U;
static bool s_initialized = false;

/**
 * @brief 由目标采样率计算定时器周期计数。
 * @param sample_rate_hz 目标采样率，单位 Hz。
 * @param timer_count 输出的定时器周期计数，范围 1~timer_max_count。
 * @param configured_trigger_rate_hz 输出由整数 Timer 周期推导出的配置触发率，单位 Hz。
 * @return 参数有效且可由定时器实现时返回 SIGNAL_RESULT_OK。
 */
static signal_result_t SignalADC_CalculateTimerCount(uint32_t sample_rate_hz,
    uint32_t *timer_count, uint32_t *configured_trigger_rate_hz)
{
    uint32_t rounded_count;

    if ((sample_rate_hz == 0U) || (timer_count == NULL) ||
        (configured_trigger_rate_hz == NULL) || (s_timer_clock_hz == 0U) ||
        (s_timer_max_count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    if (sample_rate_hz > s_timer_clock_hz) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }

    rounded_count =
        (s_timer_clock_hz + (sample_rate_hz / 2U)) / sample_rate_hz;
    if ((rounded_count == 0U) || (rounded_count > s_timer_max_count)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }

    *timer_count = rounded_count;
    *configured_trigger_rate_hz = s_timer_clock_hz / rounded_count;
    return SIGNAL_RESULT_OK;
}

/**
 * @brief 初始化模块私有状态并设置初始采样率。
 * @param config 定时器计数时钟、最大周期计数和目标采样率。
 * @return 成功返回 SIGNAL_RESULT_OK；参数或采样率不可实现时返回错误码。
 * @note 单位均为 Hz 或 Timer count；调用前必须完成 SysConfig 初始化。
 */
signal_result_t SignalADC_Init(const signal_adc_dma_config_t *config)
{
    signal_result_t result;

    if ((config == NULL) || (config->timer_clock_hz == 0U) ||
        (config->timer_max_count == 0U)) {
        s_status = MODULE_ERROR;
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    DL_TimerG_stopCounter(SIGNAL_SAMPLE_TIMER_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_INST);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_DMA_CHAN_ID);

    s_timer_clock_hz = config->timer_clock_hz;
    s_timer_max_count = config->timer_max_count;
    s_buffer = NULL;
    s_sample_count = 0U;
    s_configured_trigger_rate_hz = 0U;
    s_initialized = true;
    s_status = MODULE_IDLE;

    NVIC_ClearPendingIRQ(SIGNAL_ADC_INST_INT_IRQN);
    NVIC_EnableIRQ(SIGNAL_ADC_INST_INT_IRQN);

    result = SignalADC_SetSampleRate(config->sample_rate_hz);
    if (result != SIGNAL_RESULT_OK) {
        s_initialized = false;
        s_status = MODULE_ERROR;
    }

    return result;
}

/**
 * @brief 在 Timer 停止时更新周期寄存器和配置事件触发率。
 * @param sample_rate_hz 目标采样率，单位 Hz，范围由 Timer 时钟/位宽决定。
 * @return 成功返回 SIGNAL_RESULT_OK；采集中或超范围时返回错误码。
 * @note 不修改 ADC 通道、参考电压或 DMA 配置。
 */
signal_result_t SignalADC_SetSampleRate(uint32_t sample_rate_hz)
{
    signal_result_t result;
    uint32_t timer_count;
    uint32_t configured_trigger_rate_hz;

    if (!s_initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    if (s_status == MODULE_RUNNING) {
        return SIGNAL_RESULT_BUSY;
    }

    result = SignalADC_CalculateTimerCount(
        sample_rate_hz, &timer_count, &configured_trigger_rate_hz);
    if (result != SIGNAL_RESULT_OK) {
        return result;
    }

    DL_TimerG_stopCounter(SIGNAL_SAMPLE_TIMER_INST);
    DL_TimerG_setLoadValue(SIGNAL_SAMPLE_TIMER_INST, timer_count - 1U);
    DL_TimerG_setTimerCount(SIGNAL_SAMPLE_TIMER_INST, timer_count - 1U);
    /*
     * 该值是由 Timer 计数时钟和整数周期推导出的配置事件频率。
     * 它不是外部仪器实测的 ADC 物理采样率。
     */
    s_configured_trigger_rate_hz = configured_trigger_rate_hz;

    return SIGNAL_RESULT_OK;
}

/**
 * @brief 配置 DMA 目标与长度，并依次启动 ADC DMA 和采样 Timer。
 * @param buffer 用户提供的 uint16_t 缓冲区。
 * @param sample_count 采样点数，范围 1~65535。
 * @return 成功返回 SIGNAL_RESULT_OK；未初始化、忙或参数错误时返回错误码。
 * @note 完成前不能释放或复用 buffer。
 */
signal_result_t SignalADC_Start(uint16_t *buffer, uint16_t sample_count)
{
    if (!s_initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    if (s_status == MODULE_RUNNING) {
        return SIGNAL_RESULT_BUSY;
    }
    if ((buffer == NULL) || (sample_count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    DL_TimerG_stopCounter(SIGNAL_SAMPLE_TIMER_INST);
    DL_TimerG_setTimerCount(SIGNAL_SAMPLE_TIMER_INST,
        DL_TimerG_getLoadValue(SIGNAL_SAMPLE_TIMER_INST));

    DL_ADC12_disableConversions(SIGNAL_ADC_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_INST);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_DMA_CHAN_ID);
    /* CPU DMA IRQ 未使能，但重启前仍清原始通道完成位。 */
    DL_DMA_clearInterruptStatus(DMA, SIGNAL_ADC_DMA_INTERRUPT_MASK);

    DL_DMA_setSrcAddr(DMA, SIGNAL_ADC_DMA_CHAN_ID,
        (uint32_t) DL_ADC12_getMemResultAddress(
            SIGNAL_ADC_INST, SIGNAL_ADC_ADCMEM_0));
    DL_DMA_setDestAddr(
        DMA, SIGNAL_ADC_DMA_CHAN_ID, (uint32_t) &buffer[0]);
    DL_DMA_setTransferSize(DMA, SIGNAL_ADC_DMA_CHAN_ID, sample_count);

    DL_ADC12_clearInterruptStatus(
        SIGNAL_ADC_INST, DL_ADC12_INTERRUPT_DMA_DONE);
    NVIC_ClearPendingIRQ(SIGNAL_ADC_INST_INT_IRQN);

    s_buffer = buffer;
    s_sample_count = sample_count;
    s_status = MODULE_RUNNING;

    DL_DMA_enableChannel(DMA, SIGNAL_ADC_DMA_CHAN_ID);
    DL_ADC12_enableDMA(SIGNAL_ADC_INST);
    DL_ADC12_enableConversions(SIGNAL_ADC_INST);
    DL_TimerG_startCounter(SIGNAL_SAMPLE_TIMER_INST);

    return SIGNAL_RESULT_OK;
}

/**
 * @brief 停止 Timer、ADC 转换和 DMA，并恢复 MODULE_IDLE。
 * @return 无。
 * @note 未初始化时直接返回；不会清空用户缓冲区。
 */
void SignalADC_Stop(void)
{
    if (!s_initialized) {
        return;
    }

    DL_TimerG_stopCounter(SIGNAL_SAMPLE_TIMER_INST);
    DL_ADC12_disableConversions(SIGNAL_ADC_INST);
    DL_ADC12_disableDMA(SIGNAL_ADC_INST);
    DL_DMA_disableChannel(DMA, SIGNAL_ADC_DMA_CHAN_ID);

    s_status = MODULE_IDLE;
}

/**
 * @brief 查询 DMA 是否已完成整帧采集。
 * @return 状态为 MODULE_DONE 时返回 true，否则返回 false。
 */
bool SignalADC_IsFinished(void)
{
    return (s_status == MODULE_DONE);
}

/**
 * @brief 查询模块状态。
 * @return IDLE、RUNNING、DONE 或 ERROR。
 */
signal_status_t SignalADC_GetStatus(void)
{
    return s_status;
}

/**
 * @brief 返回最近一次 Start 接收的用户缓冲区。
 * @return 只读指针；从未启动时为 NULL。
 * @note 仅在 MODULE_DONE 后读取才能保证整帧有效。
 */
const uint16_t *SignalADC_GetBuffer(void)
{
    return s_buffer;
}

/**
 * @brief 返回最近一次 Start 的采样点数。
 * @return 点数，范围 0~65535；0 表示从未启动。
 */
uint16_t SignalADC_GetSampleCount(void)
{
    return s_sample_count;
}

/**
 * @brief 返回整数 Timer 周期推导出的配置事件触发率。
 * @return 配置触发率，单位 Hz；未成功初始化时为 0。
 * @note 此值不是外部仪器对 ADC 物理采样率的实测结果。
 */
uint32_t SignalADC_GetConfiguredTriggerRate(void)
{
    return s_configured_trigger_rate_hz;
}

signal_module_status_t SignalADC_GetModuleMaturity(void)
{
    return MODULE_STATUS_BOARD_VERIFIED;
}

/**
 * @brief ADC DMA 完成中断，只停止硬件并更新状态，不做耗时运算。
 * @return 无。
 */
void SIGNAL_ADC_INST_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(SIGNAL_ADC_INST)) {
        case DL_ADC12_IIDX_DMA_DONE:
            DL_TimerG_stopCounter(SIGNAL_SAMPLE_TIMER_INST);
            DL_ADC12_disableConversions(SIGNAL_ADC_INST);
            DL_ADC12_disableDMA(SIGNAL_ADC_INST);
            DL_DMA_disableChannel(DMA, SIGNAL_ADC_DMA_CHAN_ID);
            s_status = MODULE_DONE;
            break;
        default:
            break;
    }
}
