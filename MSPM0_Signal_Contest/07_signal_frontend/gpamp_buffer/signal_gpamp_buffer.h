#ifndef SIGNAL_GPAMP_BUFFER_H
#define SIGNAL_GPAMP_BUFFER_H

#include "signal_gpamp.h"

signal_result_t SignalGPAMPBuffer_MakeConfig(float bias_voltage_v,
    signal_gpamp_config_t *config);
signal_module_status_t SignalGPAMPBuffer_GetModuleStatus(void);

#endif
