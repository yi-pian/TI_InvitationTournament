#include "signal_adc_dual_sync.h"

#include <stddef.h>

signal_result_t SignalADCDualSync_Deinterleave(const uint16_t *interleaved,
    size_t pair_count, uint16_t *channel_a, size_t channel_a_capacity,
    uint16_t *channel_b, size_t channel_b_capacity)
{
    size_t index;
    if ((interleaved == NULL) || (channel_a == NULL) || (channel_b == NULL) ||
        (pair_count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if ((channel_a_capacity < pair_count) || (channel_b_capacity < pair_count)) {
        return SIGNAL_RESULT_INSUFFICIENT_BUFFER;
    }
    for (index = 0U; index < pair_count; ++index) {
        channel_a[index] = interleaved[2U * index];
        channel_b[index] = interleaved[(2U * index) + 1U];
    }
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalADCDualSync_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
