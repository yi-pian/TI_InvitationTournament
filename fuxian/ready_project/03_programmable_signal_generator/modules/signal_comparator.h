#ifndef SIGNAL_COMPARATOR_H
#define SIGNAL_COMPARATOR_H

#include <stdbool.h>
#include "signal_status.h"

typedef struct {
    float threshold_v;
    float hysteresis_v;
    bool invert_output;
} signal_comparator_config_t;

typedef signal_result_t (*signal_comparator_apply_fn)(void *context,
    const signal_comparator_config_t *config);
typedef struct {
    void *context;
    signal_comparator_apply_fn apply;
} signal_comparator_t;

signal_result_t SignalComparator_ValidateConfig(
    const signal_comparator_config_t *config, float supply_voltage_v);
signal_result_t SignalComparator_Apply(const signal_comparator_t *comparator,
    const signal_comparator_config_t *config, float supply_voltage_v);
signal_module_status_t SignalComparator_GetModuleStatus(void);

#endif
