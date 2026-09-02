#include "signal_am_modulation.h"

#include <stddef.h>

signal_result_t SignalAMModulation_Apply(const float *carrier,
    const float *message, size_t count, float modulation_index,
    float *output, size_t output_capacity)
{
    size_t index;
    if ((carrier == NULL) || (message == NULL) || (output == NULL) ||
        (count == 0U) || (modulation_index < 0.0f) ||
        (modulation_index > 1.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (output_capacity < count) { return SIGNAL_RESULT_INSUFFICIENT_BUFFER; }
    for (index = 0U; index < count; ++index) {
        output[index] = carrier[index] *
            (1.0f + modulation_index * message[index]);
    }
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalAMModulation_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
