#ifndef SIGNAL_MSPM0G3507_CAPTURE_PLATFORM_H
#define SIGNAL_MSPM0G3507_CAPTURE_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "signal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    volatile uint32_t *timestamps;
    size_t capacity;
    uint32_t counter_modulus;
    uint32_t timeout_overflows;
    volatile size_t count;
    volatile uint32_t overflow_count;
    volatile bool finished;
    bool initialized;
} signal_mspm0g3507_capture_t;

/* Requires generated SIGNAL_CAPTURE_INST / IRQ macros from PROFILE_05/06. */
signal_result_t SignalMSPM0G3507_Capture_Init(
    signal_mspm0g3507_capture_t *capture,
    volatile uint32_t *timestamps,
    size_t capacity,
    uint32_t counter_modulus,
    uint32_t timeout_overflows);
signal_result_t SignalMSPM0G3507_Capture_Start(
    signal_mspm0g3507_capture_t *capture);
signal_result_t SignalMSPM0G3507_Capture_Stop(
    signal_mspm0g3507_capture_t *capture);
bool SignalMSPM0G3507_Capture_IsFinished(
    const signal_mspm0g3507_capture_t *capture);
size_t SignalMSPM0G3507_Capture_GetCount(
    const signal_mspm0g3507_capture_t *capture);
signal_result_t SignalMSPM0G3507_Capture_Copy(
    const signal_mspm0g3507_capture_t *capture,
    uint32_t *destination,
    size_t capacity,
    size_t *copied);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_MSPM0G3507_CAPTURE_PLATFORM_H */
