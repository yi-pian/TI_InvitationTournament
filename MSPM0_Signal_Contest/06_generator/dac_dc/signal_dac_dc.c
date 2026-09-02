#include "signal_dac_dc.h"

#include <stddef.h>

signal_result_t SignalDACDC_SetVoltage(const signal_dac_t *dac,
    float voltage_v, uint16_t *written_raw)
{
    uint16_t raw;
    signal_result_t result;
    if ((dac == NULL) || (written_raw == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    result = SignalDAC_VoltageToRaw(voltage_v, dac->resolution_bits,
        dac->reference_voltage_v, &raw);
    if (result != SIGNAL_RESULT_OK) { return result; }
    result = SignalDAC_WriteRaw(dac, raw);
    if (result == SIGNAL_RESULT_OK) { *written_raw = raw; }
    return result;
}

signal_module_status_t SignalDACDC_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
