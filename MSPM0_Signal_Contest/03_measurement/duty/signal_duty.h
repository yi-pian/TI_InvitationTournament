#ifndef SIGNAL_DUTY_H
#define SIGNAL_DUTY_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef enum
{
    SIGNAL_DUTY_LEVELS_AUTO_MIN_MAX = 0,
    SIGNAL_DUTY_LEVELS_EXPLICIT = 1
} signal_duty_level_mode_t;

typedef struct
{
    signal_duty_level_mode_t level_mode;
    float threshold_ratio;
    float hysteresis_ratio;
    float min_amplitude;
    float low_level;
    float high_level;
} signal_duty_config_t;

typedef struct
{
    float duty_ratio;
    float duty_percent;
    float period_s;
    float frequency_hz;
    float high_width_s;
    float low_width_s;
    float low_level;
    float high_level;
    float threshold_level;
    uint32_t valid_cycle_count;
    uint32_t rising_edge_count;
    uint32_t falling_edge_count;
} signal_duty_result_t;

/** Fill a configuration with 50% threshold, 5% hysteresis and auto min/max levels. */
signal_algorithm_status_t SignalDuty_GetDefaultConfig(signal_duty_config_t *config);

/**
 * Measure positive duty from complete rising-falling-rising cycles.
 *
 * @param samples Read-only finite samples in any linear amplitude unit.
 * @param count Number of samples; at least three and enough for one full cycle.
 * @param sample_rate_hz Physical sample rate in Hz, finite and greater than zero.
 * @param config Threshold, hysteresis and state-level configuration.
 * @param result Output in ratio, percent, seconds and Hz. Unchanged on failure.
 */
signal_algorithm_status_t SignalDuty_Process(
    const float *samples,
    uint32_t count,
    float sample_rate_hz,
    const signal_duty_config_t *config,
    signal_duty_result_t *result);

#endif /* SIGNAL_DUTY_H */
