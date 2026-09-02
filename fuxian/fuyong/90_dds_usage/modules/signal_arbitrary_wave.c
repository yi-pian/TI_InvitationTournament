#include "signal_arbitrary_wave.h"

#include <stddef.h>

signal_result_t SignalArbitraryWave_ResampleLinear(const uint16_t *source,
    size_t source_count, uint16_t *destination, size_t destination_count)
{
    size_t index;
    if ((source == NULL) || (destination == NULL) || (source_count < 2U) ||
        (destination_count < 2U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    for (index = 0U; index < destination_count; ++index) {
        float position = (float) index * (float) source_count /
            (float) destination_count;
        size_t left = (size_t) position;
        size_t right = left + 1U;
        float fraction = position - (float) left;
        if (right >= source_count) { right = 0U; }
        destination[index] = (uint16_t) ((1.0f - fraction) * source[left] +
            fraction * source[right] + 0.5f);
    }
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalArbitraryWave_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
