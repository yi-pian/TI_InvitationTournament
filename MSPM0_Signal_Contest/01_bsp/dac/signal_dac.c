#include "signal_dac.h"

#include <stddef.h>

signal_result_t SignalDAC_VoltageToRaw(float voltage_v, uint8_t bits,
    float reference_voltage_v, uint16_t *raw)
{
    uint32_t full_scale;
    float scaled;
    if ((raw == NULL) || (bits == 0U) || (bits > 16U) ||
        !(reference_voltage_v > 0.0f) || (voltage_v < 0.0f) ||
        (voltage_v > reference_voltage_v)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    full_scale = (1UL << bits) - 1UL;
    scaled = (voltage_v / reference_voltage_v) * (float) full_scale;
    *raw = (uint16_t) (scaled + 0.5f);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalDAC_WriteRaw(const signal_dac_t *dac, uint16_t raw)
{
    uint32_t full_scale;
    if ((dac == NULL) || (dac->write == NULL) ||
        (dac->resolution_bits == 0U) || (dac->resolution_bits > 16U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    full_scale = (1UL << dac->resolution_bits) - 1UL;
    if ((uint32_t) raw > full_scale) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    return dac->write(dac->context, raw);
}

signal_module_status_t SignalDAC_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
