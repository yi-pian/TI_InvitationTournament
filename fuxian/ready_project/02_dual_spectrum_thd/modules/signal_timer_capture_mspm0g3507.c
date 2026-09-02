#include "signal_timer_capture_mspm0g3507.h"

#include <stddef.h>

#include "ti_msp_dl_config.h"

static signal_timer_capture_mspm0_config_t s_config;
static volatile uint32_t s_period_ticks;
static volatile uint32_t s_high_ticks;
static volatile uint32_t s_sequence;
static volatile bool s_valid;
static volatile bool s_synchronized;
#if defined(SIGNAL_CAPTURE_INST_SUB_0_CH)
/* Trigger Capture 模式：COMP 事件把每个边沿捕获到 CC0。 */
static volatile uint32_t s_last_edge_capture;
static volatile bool s_have_edge_capture;
#endif
static bool s_initialized;

signal_result_t SignalTimerCapture_MSPM0_Init(
    const signal_timer_capture_mspm0_config_t *config)
{
    if ((config == NULL) || (config->timer_clock_hz == 0U) ||
        (config->load_value < 2U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    DL_TimerG_stopCounter(SIGNAL_CAPTURE_INST);
    NVIC_DisableIRQ(SIGNAL_CAPTURE_INST_INT_IRQN);
    s_config = *config;
    s_period_ticks = 0U;
    s_high_ticks = 0U;
    s_sequence = 0U;
    s_valid = false;
    s_synchronized = false;
#if defined(SIGNAL_CAPTURE_INST_SUB_0_CH)
    s_last_edge_capture = 0U;
    s_have_edge_capture = false;
#endif
    s_initialized = true;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalTimerCapture_MSPM0_Start(void)
{
    if (!s_initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }

    DL_TimerG_stopCounter(SIGNAL_CAPTURE_INST);
    DL_TimerG_setTimerCount(SIGNAL_CAPTURE_INST, s_config.load_value);
    s_valid = false;
    s_synchronized = false;
#if defined(SIGNAL_CAPTURE_INST_SUB_0_CH)
    s_last_edge_capture = 0U;
    s_have_edge_capture = false;
#endif
    NVIC_ClearPendingIRQ(SIGNAL_CAPTURE_INST_INT_IRQN);
    NVIC_EnableIRQ(SIGNAL_CAPTURE_INST_INT_IRQN);
    DL_TimerG_startCounter(SIGNAL_CAPTURE_INST);
    return SIGNAL_RESULT_OK;
}

void SignalTimerCapture_MSPM0_Stop(void)
{
    DL_TimerG_stopCounter(SIGNAL_CAPTURE_INST);
    NVIC_DisableIRQ(SIGNAL_CAPTURE_INST_INT_IRQN);
    s_valid = false;
}

signal_result_t SignalTimerCapture_MSPM0_GetResult(
    signal_timer_capture_mspm0_result_t *result)
{
    uint32_t before;
    uint32_t after;
    uint32_t period;
    uint32_t high;
    bool valid;

    if (!s_initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    if (result == NULL) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    do {
        before = s_sequence;
        period = s_period_ticks;
        high = s_high_ticks;
        valid = s_valid;
        after = s_sequence;
    } while ((before != after) || ((before & 1U) != 0U));

    result->period_ticks = period;
    result->high_ticks = high;
    result->valid = valid && (period != 0U) && (high <= period);
    if (!result->valid) {
        result->frequency_hz = 0.0F;
        result->duty_percent = 0.0F;
        return SIGNAL_RESULT_NO_DATA;
    }

    result->frequency_hz =
        (float)s_config.timer_clock_hz / (float)period;
    result->duty_percent = 100.0F * (float)high / (float)period;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalTimerCapture_MSPM0_GetModuleMaturity(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}

void SIGNAL_CAPTURE_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(SIGNAL_CAPTURE_INST)) {
#if defined(SIGNAL_CAPTURE_INST_SUB_0_CH)
        case DL_TIMERG_IIDX_CC0_DN:
        {
            uint32_t capture = DL_TimerG_getCaptureCompareValue(
                SIGNAL_CAPTURE_INST, DL_TIMER_CC_0_INDEX);
            if (s_have_edge_capture) {
                uint32_t modulus = s_config.load_value + 1U;
                uint32_t period = (s_last_edge_capture >= capture) ?
                    (s_last_edge_capture - capture) :
                    (s_last_edge_capture + modulus - capture);
                if (period != 0U) {
                    s_sequence++;
                    s_period_ticks = period;
                    s_high_ticks = 0U;
                    s_valid = true;
                    s_sequence++;
                }
            }
            s_last_edge_capture = capture;
            s_have_edge_capture = true;
            break;
        }
#endif
        case DL_TIMERG_IIDX_CC1_DN:
#if defined(SIGNAL_CAPTURE_INST_SUB_0_CH)
            /* Trigger Capture 不使用 CC1；保留分支以兼容旧生成配置。 */
            break;
#else
            if (s_synchronized) {
                uint32_t period_capture = DL_TimerG_getCaptureCompareValue(
                    SIGNAL_CAPTURE_INST, DL_TIMER_CC_1_INDEX);
                uint32_t high_capture = DL_TimerG_getCaptureCompareValue(
                    SIGNAL_CAPTURE_INST, DL_TIMER_CC_0_INDEX);
                uint32_t period = s_config.load_value - period_capture;
                uint32_t high = s_config.load_value - high_capture;

                s_sequence++;
                s_period_ticks = period;
                s_high_ticks = high;
                s_valid = (period != 0U) && (high <= period);
                s_sequence++;
            } else {
                s_synchronized = true;
            }

            /* TIMER_ERR_01: combined capture requires a manual reload. */
            DL_TimerG_setTimerCount(SIGNAL_CAPTURE_INST, s_config.load_value);
            break;
#endif

        case DL_TIMERG_IIDX_ZERO:
#if defined(SIGNAL_CAPTURE_INST_SUB_0_CH)
            /* 触发捕获的边沿时间通过模运算处理回绕，ZERO 不清除有效周期。 */
            break;
#else
            s_sequence++;
            s_valid = false;
            s_synchronized = false;
            s_sequence++;
#endif
            break;

        default:
            break;
    }
}
