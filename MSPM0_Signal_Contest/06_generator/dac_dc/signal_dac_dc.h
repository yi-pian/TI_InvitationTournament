#ifndef SIGNAL_DAC_DC_H
#define SIGNAL_DAC_DC_H

#include "signal_dac.h"
#include "signal_status.h"

signal_result_t SignalDACDC_SetVoltage(const signal_dac_t *dac,
    float voltage_v, uint16_t *written_raw);
signal_module_status_t SignalDACDC_GetModuleStatus(void);

#endif
