#ifndef SIGNAL_GPAMP_GAIN_H
#define SIGNAL_GPAMP_GAIN_H

#include "signal_gpamp.h"

signal_result_t SignalGPAMPGain_MakeConfig(float requested_gain,
    float bias_voltage_v, signal_gpamp_config_t *config);
signal_module_status_t SignalGPAMPGain_GetModuleStatus(void);

#endif
