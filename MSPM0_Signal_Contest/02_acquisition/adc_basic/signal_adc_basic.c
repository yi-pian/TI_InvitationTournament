#include "signal_adc_basic.h"

#include <stddef.h>

signal_result_t SignalADCBasic_ReadBlock(const signal_adc_t *adc,
    uint16_t *destination, size_t sample_count)
{
    size_t index;
    signal_result_t result;
    if ((adc == NULL) || (destination == NULL) || (sample_count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    for (index = 0U; index < sample_count; ++index) {
        result = SignalADC_ReadRaw(adc, &destination[index]);
        if (result != SIGNAL_RESULT_OK) {
            return result;
        }
    }
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalADCBasic_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
