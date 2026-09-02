#ifndef SIGNAL_SLEW_RATE_H
#define SIGNAL_SLEW_RATE_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct {
    float low_ratio;
    float high_ratio;
} signal_slew_rate_config_t;

typedef struct {
    float rise_time_us;
    float fall_time_us;
    uint32_t rise_count;
    uint32_t fall_count;
} signal_slew_rate_result_t;

signal_algorithm_status_t SignalSlewRate_Process(
    const float *samples, uint32_t sample_count, float low_voltage_v,
    float high_voltage_v, uint32_t sample_rate_hz,
    const signal_slew_rate_config_t *config,
    signal_slew_rate_result_t *result);

#endif
