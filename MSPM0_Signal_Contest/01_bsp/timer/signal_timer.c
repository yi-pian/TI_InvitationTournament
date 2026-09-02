#include "signal_timer.h"

#include <stddef.h>
#include "signal_system_clock.h"

signal_result_t SignalTimer_SetRate(const signal_timer_t *timer,
    uint32_t requested_rate_hz, uint32_t *configured_rate_hz)
{
    uint32_t count;
    signal_result_t result;
    if ((timer == NULL) || (timer->set_period_count == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    result = SignalSystemClock_CalculateTimerPeriod(timer->clock_hz,
        requested_rate_hz, timer->max_count, &count, configured_rate_hz);
    if (result != SIGNAL_RESULT_OK) {
        return result;
    }
    return timer->set_period_count(timer->context, count);
}

signal_result_t SignalTimer_Start(const signal_timer_t *timer)
{
    if ((timer == NULL) || (timer->start == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return timer->start(timer->context);
}

signal_result_t SignalTimer_Stop(const signal_timer_t *timer)
{
    if ((timer == NULL) || (timer->stop == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return timer->stop(timer->context);
}

signal_result_t SignalTimer_ReadCount(const signal_timer_t *timer,
    uint32_t *count)
{
    if ((timer == NULL) || (timer->read_count == NULL) || (count == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return timer->read_count(timer->context, count);
}

signal_module_status_t SignalTimer_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
