#ifndef SIGNAL_TIMER_CAPTURE_MSPM0G3507_H
#define SIGNAL_TIMER_CAPTURE_MSPM0G3507_H

#include <stdbool.h>
#include <stdint.h>

#include "signal_status.h"

typedef struct {
    uint32_t timer_clock_hz;
    uint32_t load_value;
} signal_timer_capture_mspm0_config_t;

typedef struct {
    uint32_t period_ticks;
    uint32_t high_ticks;
    float frequency_hz;
    float duty_percent;
    bool valid;
} signal_timer_capture_mspm0_result_t;

/* TIMG combined capture: CCP0 receives the externally shaped logic signal. */
signal_result_t SignalTimerCapture_MSPM0_Init(
    const signal_timer_capture_mspm0_config_t *config);
signal_result_t SignalTimerCapture_MSPM0_Start(void);
void SignalTimerCapture_MSPM0_Stop(void);
signal_result_t SignalTimerCapture_MSPM0_GetResult(
    signal_timer_capture_mspm0_result_t *result);
signal_module_status_t SignalTimerCapture_MSPM0_GetModuleMaturity(void);

#endif /* SIGNAL_TIMER_CAPTURE_MSPM0G3507_H */
