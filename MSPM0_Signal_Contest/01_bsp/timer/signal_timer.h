#ifndef SIGNAL_TIMER_H
#define SIGNAL_TIMER_H

#include <stdbool.h>
#include <stdint.h>
#include "signal_status.h"

typedef signal_result_t (*signal_timer_set_count_fn)(void *context,
    uint32_t count);
typedef signal_result_t (*signal_timer_control_fn)(void *context);
typedef signal_result_t (*signal_timer_read_fn)(void *context,
    uint32_t *count);

typedef struct {
    void *context;
    signal_timer_set_count_fn set_period_count;
    signal_timer_control_fn start;
    signal_timer_control_fn stop;
    signal_timer_read_fn read_count;
    uint32_t clock_hz;
    uint32_t max_count;
} signal_timer_t;

signal_result_t SignalTimer_SetRate(const signal_timer_t *timer,
    uint32_t requested_rate_hz, uint32_t *configured_rate_hz);
signal_result_t SignalTimer_Start(const signal_timer_t *timer);
signal_result_t SignalTimer_Stop(const signal_timer_t *timer);
signal_result_t SignalTimer_ReadCount(const signal_timer_t *timer,
    uint32_t *count);
signal_module_status_t SignalTimer_GetModuleStatus(void);

#endif
