#include "signal_adc.h"

#include <stddef.h>

signal_result_t SignalADC_ValidateConfig(const signal_adc_config_t *config)
{
    if ((config == NULL) || (config->resolution_bits < 8U) ||
        (config->resolution_bits > 14U) ||
        !(config->reference_voltage_v > 0.0f) || (config->clock_hz == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalADC_ReadRaw(const signal_adc_t *adc, uint16_t *raw)
{
    signal_result_t result;
    if ((adc == NULL) || (adc->read == NULL) || (raw == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    result = SignalADC_ValidateConfig(&adc->config);
    return (result == SIGNAL_RESULT_OK) ? adc->read(adc->context, raw) : result;
}

signal_module_status_t SignalADC_GetBspModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
