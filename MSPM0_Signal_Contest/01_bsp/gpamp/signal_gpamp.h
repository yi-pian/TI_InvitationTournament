#ifndef SIGNAL_GPAMP_H
#define SIGNAL_GPAMP_H

#include "signal_status.h"

typedef struct {
    float requested_gain;
    float bias_voltage_v;
} signal_gpamp_config_t;

typedef signal_result_t (*signal_gpamp_apply_fn)(void *context,
    const signal_gpamp_config_t *config);
typedef struct { void *context; signal_gpamp_apply_fn apply; } signal_gpamp_t;

signal_result_t SignalGPAMP_ValidateConfig(const signal_gpamp_config_t *config);
signal_result_t SignalGPAMP_Apply(const signal_gpamp_t *gpamp,
    const signal_gpamp_config_t *config);
signal_module_status_t SignalGPAMP_GetModuleStatus(void);

#endif
