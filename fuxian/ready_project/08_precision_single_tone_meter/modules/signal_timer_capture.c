#include "signal_timer_capture.h"

#include <stddef.h>

signal_result_t SignalTimerCapture_Delta(uint32_t earlier, uint32_t later,
    uint32_t counter_modulus, uint32_t *delta_ticks)
{
    if ((counter_modulus == 0U) || (delta_ticks == NULL) ||
        (earlier >= counter_modulus) || (later >= counter_modulus)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *delta_ticks = (later >= earlier) ? (later - earlier) :
        (counter_modulus - earlier + later);
    return (*delta_ticks == 0U) ? SIGNAL_RESULT_NO_DATA : SIGNAL_RESULT_OK;
}

signal_result_t SignalTimerCapture_MeanPeriod(const uint32_t *timestamps,
    size_t timestamp_count, const signal_timer_capture_config_t *config,
    float *mean_ticks, float *frequency_hz)
{
    size_t index;
    uint32_t delta;
    uint64_t sum = 0U;
    signal_result_t result;
    if ((timestamps == NULL) || (config == NULL) || (mean_ticks == NULL) ||
        (frequency_hz == NULL) || (timestamp_count < 2U) ||
        (config->timer_hz == 0U) || (config->counter_modulus == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    for (index = 1U; index < timestamp_count; ++index) {
        result = SignalTimerCapture_Delta(timestamps[index - 1U],
            timestamps[index], config->counter_modulus, &delta);
        if (result != SIGNAL_RESULT_OK) {
            return result;
        }
        sum += delta;
    }
    *mean_ticks = (float) sum / (float) (timestamp_count - 1U);
    *frequency_hz = (float) config->timer_hz / *mean_ticks;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalTimerCapture_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
