#ifndef PERIPHERAL_SYSTEM_TEMPLATE_H
#define PERIPHERAL_SYSTEM_TEMPLATE_H

#include <stdbool.h>
#include <stdint.h>

#include "signal_status.h"
#include "signal_types.h"

typedef signal_result_t (*peripheral_acquire_fn)(
    void *context, signal_u16_frame_t *frame);

typedef signal_result_t (*peripheral_algorithm_hook_fn)(
    void *context, const signal_u16_frame_t *frame);

typedef signal_result_t (*peripheral_output_fn)(
    void *context, const signal_u16_frame_t *frame);

typedef struct {
    void *acquire_context;
    peripheral_acquire_fn acquire;
    void *algorithm_context;
    peripheral_algorithm_hook_fn algorithm_hook;
    void *output_context;
    peripheral_output_fn output;
} peripheral_system_config_t;

typedef struct {
    peripheral_system_config_t config;
    uint32_t completed_frames;
    signal_result_t last_result;
    bool initialized;
} peripheral_system_t;

signal_result_t PeripheralSystem_Init(
    peripheral_system_t *system, const peripheral_system_config_t *config);

signal_result_t PeripheralSystem_RunOnce(peripheral_system_t *system);

#endif /* PERIPHERAL_SYSTEM_TEMPLATE_H */
