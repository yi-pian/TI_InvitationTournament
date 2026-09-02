#include "signal_mspm0g3507_capture_platform.h"

#include "ti_msp_dl_config.h"

static signal_mspm0g3507_capture_t *s_active_capture;

signal_result_t SignalMSPM0G3507_Capture_Init(
    signal_mspm0g3507_capture_t *capture,
    volatile uint32_t *timestamps,
    size_t capacity,
    uint32_t counter_modulus,
    uint32_t timeout_overflows)
{
    if ((capture == NULL) || (timestamps == NULL) || (capacity < 2U) ||
        (counter_modulus < 2U) || (timeout_overflows == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    capture->timestamps = timestamps;
    capture->capacity = capacity;
    capture->counter_modulus = counter_modulus;
    capture->timeout_overflows = timeout_overflows;
    capture->count = 0U;
    capture->overflow_count = 0U;
    capture->finished = false;
    capture->initialized = true;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_Capture_Start(
    signal_mspm0g3507_capture_t *capture)
{
    if ((capture == NULL) || !capture->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    if ((s_active_capture != NULL) && (s_active_capture != capture)) {
        return SIGNAL_RESULT_BUSY;
    }
    capture->count = 0U;
    capture->overflow_count = 0U;
    capture->finished = false;
    s_active_capture = capture;
    DL_TimerG_stopCounter(SIGNAL_CAPTURE_INST);
    DL_TimerG_setTimerCount(SIGNAL_CAPTURE_INST,
        DL_TimerG_getLoadValue(SIGNAL_CAPTURE_INST));
    NVIC_ClearPendingIRQ(SIGNAL_CAPTURE_INST_INT_IRQN);
    NVIC_EnableIRQ(SIGNAL_CAPTURE_INST_INT_IRQN);
    DL_TimerG_startCounter(SIGNAL_CAPTURE_INST);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_Capture_Stop(
    signal_mspm0g3507_capture_t *capture)
{
    if ((capture == NULL) || !capture->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    DL_TimerG_stopCounter(SIGNAL_CAPTURE_INST);
    NVIC_DisableIRQ(SIGNAL_CAPTURE_INST_INT_IRQN);
    if (s_active_capture == capture) s_active_capture = NULL;
    return SIGNAL_RESULT_OK;
}

bool SignalMSPM0G3507_Capture_IsFinished(
    const signal_mspm0g3507_capture_t *capture)
{
    return (capture != NULL) && capture->initialized && capture->finished;
}

size_t SignalMSPM0G3507_Capture_GetCount(
    const signal_mspm0g3507_capture_t *capture)
{
    return ((capture != NULL) && capture->initialized) ? capture->count : 0U;
}

signal_result_t SignalMSPM0G3507_Capture_Copy(
    const signal_mspm0g3507_capture_t *capture,
    uint32_t *destination,
    size_t capacity,
    size_t *copied)
{
    size_t index;
    size_t count;
    if ((capture == NULL) || !capture->initialized ||
        (destination == NULL) || (copied == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    count = capture->count;
    if (capacity < count) return SIGNAL_RESULT_INSUFFICIENT_BUFFER;
    for (index = 0U; index < count; ++index) {
        destination[index] = capture->timestamps[index];
    }
    *copied = count;
    return (count == 0U) ? SIGNAL_RESULT_NO_DATA : SIGNAL_RESULT_OK;
}

void SIGNAL_CAPTURE_INST_IRQHandler(void)
{
    signal_mspm0g3507_capture_t *capture = s_active_capture;
    if (capture == NULL) {
        (void) DL_TimerG_getPendingInterrupt(SIGNAL_CAPTURE_INST);
        return;
    }
    switch (DL_TimerG_getPendingInterrupt(SIGNAL_CAPTURE_INST)) {
        case DL_TIMERG_IIDX_CC0_DN:
            if (capture->count < capture->capacity) {
                uint32_t raw_capture = DL_Timer_getCaptureCompareValue(
                    SIGNAL_CAPTURE_INST, DL_TIMER_CC_0_INDEX);
                capture->timestamps[capture->count] =
                    capture->counter_modulus - 1U - raw_capture;
                capture->count++;
                if (capture->count == capture->capacity) {
                    capture->finished = true;
                }
            }
            break;
        case DL_TIMERG_IIDX_ZERO:
            capture->overflow_count++;
            if (capture->overflow_count >= capture->timeout_overflows) {
                capture->finished = true;
            }
            break;
        default:
            break;
    }
}
