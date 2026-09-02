#include "signal_system_clock.h"

#include <stddef.h>

signal_result_t SignalSystemClock_Validate(const signal_system_clock_config_t *config)
{
    if ((config == NULL) || (config->cpu_hz == 0U) ||
        (config->bus_hz == 0U) || (config->timer_hz == 0U) ||
        (config->adc_hz == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if ((config->bus_hz > config->cpu_hz) ||
        (config->timer_hz > config->cpu_hz) ||
        (config->adc_hz > config->cpu_hz)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalSystemClock_CalculateTimerPeriod(uint32_t timer_hz,
    uint32_t requested_rate_hz, uint32_t max_count, uint32_t *timer_count,
    uint32_t *configured_rate_hz)
{
    uint32_t count;
    if ((timer_hz == 0U) || (requested_rate_hz == 0U) ||
        (max_count == 0U) || (timer_count == NULL) ||
        (configured_rate_hz == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (requested_rate_hz > timer_hz) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    count = (timer_hz + (requested_rate_hz / 2U)) / requested_rate_hz;
    if ((count == 0U) || (count > max_count)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    *timer_count = count;
    *configured_rate_hz = timer_hz / count;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalSystemClock_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
