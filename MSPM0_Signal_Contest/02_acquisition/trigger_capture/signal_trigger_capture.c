#include "signal_trigger_capture.h"

#include <stddef.h>

signal_result_t SignalTrigger_Find(const uint16_t *samples, size_t count,
    const signal_trigger_config_t *config, size_t search_start,
    size_t *trigger_index)
{
    size_t index;
    uint16_t low;
    uint16_t high;
    bool rising_armed;
    bool falling_armed;
    if ((samples == NULL) || (config == NULL) || (trigger_index == NULL) ||
        (count < 2U) || (search_start >= (count - 1U))) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    low = (config->level > config->hysteresis) ?
        (uint16_t) (config->level - config->hysteresis) : 0U;
    high = (uint16_t) (((uint32_t) config->level + config->hysteresis > 65535U) ?
        65535U : (uint32_t) config->level + config->hysteresis);
    /* 真正的迟滞状态机：先进入 low/high 一侧的 armed 状态，随后允许
     * 经过多个采样点再到达另一侧；不要求相邻两个采样点一次跨完整个迟滞区。 */
    rising_armed = (samples[search_start] <= low);
    falling_armed = (samples[search_start] >= high);
    for (index = search_start + 1U; index < count; ++index) {
        bool rising = rising_armed && (samples[index] >= high);
        bool falling = falling_armed && (samples[index] <= low);
        if (samples[index] <= low) rising_armed = true;
        if (samples[index] >= high) falling_armed = true;
        if (((config->edge == SIGNAL_TRIGGER_RISING) && rising) ||
            ((config->edge == SIGNAL_TRIGGER_FALLING) && falling) ||
            ((config->edge == SIGNAL_TRIGGER_EITHER) && (rising || falling))) {
            *trigger_index = index;
            return SIGNAL_RESULT_OK;
        }
    }
    return SIGNAL_RESULT_NO_DATA;
}

signal_result_t SignalTrigger_Extract(const uint16_t *samples, size_t count,
    size_t trigger_index, size_t pretrigger_count, uint16_t *output,
    size_t output_count)
{
    size_t start;
    size_t index;
    if ((samples == NULL) || (output == NULL) || (output_count == 0U) ||
        (trigger_index >= count) || (pretrigger_count > trigger_index)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    start = trigger_index - pretrigger_count;
    if ((start + output_count) > count) {
        return SIGNAL_RESULT_INSUFFICIENT_BUFFER;
    }
    for (index = 0U; index < output_count; ++index) {
        output[index] = samples[start + index];
    }
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalTrigger_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
