#include "signal_vref.h"

#include <stddef.h>

signal_result_t SignalVREF_GetEffectiveVoltage(
    const signal_vref_calibration_t *calibration, float *voltage_v)
{
    if ((calibration == NULL) || (voltage_v == NULL) ||
        !(calibration->nominal_voltage_v > 0.0f) ||
        (calibration->measured_voltage_v < 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *voltage_v = (calibration->measured_voltage_v > 0.0f) ?
        calibration->measured_voltage_v : calibration->nominal_voltage_v;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalVREF_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
