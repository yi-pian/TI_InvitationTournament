#ifndef SIGNAL_OPA_H
#define SIGNAL_OPA_H

#include "signal_status.h"

typedef enum {
    SIGNAL_OPA_MODE_BUFFER = 0,
    SIGNAL_OPA_MODE_NONINVERTING,
    SIGNAL_OPA_MODE_INVERTING
} signal_opa_mode_t;

typedef struct {
    signal_opa_mode_t mode;
    float resistor_feedback_ohm;
    float resistor_input_ohm;
    float bias_voltage_v;
} signal_opa_config_t;

typedef signal_result_t (*signal_opa_apply_fn)(void *context,
    const signal_opa_config_t *config);
typedef struct { void *context; signal_opa_apply_fn apply; } signal_opa_t;

signal_result_t SignalOPA_CalculateGain(const signal_opa_config_t *config,
    float *gain);
signal_result_t SignalOPA_Apply(const signal_opa_t *opa,
    const signal_opa_config_t *config);
signal_module_status_t SignalOPA_GetModuleStatus(void);

#endif
