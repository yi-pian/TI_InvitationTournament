#ifndef SIGNAL_OPA_BUFFER_H
#define SIGNAL_OPA_BUFFER_H

#include "signal_opa.h"

signal_result_t SignalOPABuffer_MakeConfig(float bias_voltage_v,
    signal_opa_config_t *config);
signal_module_status_t SignalOPABuffer_GetModuleStatus(void);

#endif
