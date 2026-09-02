/**
 * @file main_template.c
 * @brief Compile-safe application skeleton. Replace only the three adapters.
 */

#include "peripheral_system_template.h"
#include "signal_hw_config.h"

/* ========================= Hardware Acquisition =========================
 * Connect a selected profile and return one completed frame.
 */
static signal_result_t App_Acquire(
    void *context, signal_u16_frame_t *frame)
{
    (void) context;
    (void) frame;
    /* TODO: Start/wait/acquire through the chosen peripheral adapter. */
    return SIGNAL_RESULT_NOT_SUPPORTED;
}

/* ========================= Algorithm Processing =========================
 * Another task replaces this hook, without seeing DMA.
 */
static signal_result_t App_AlgorithmHook(
    void *context, const signal_u16_frame_t *frame)
{
    (void) context;
    (void) frame;
    return SIGNAL_RESULT_OK;
}

/* ========================= Output / Display =============================
 * Optional UART, DAC or display adapter.
 */
static signal_result_t App_Output(
    void *context, const signal_u16_frame_t *frame)
{
    (void) context;
    (void) frame;
    return SIGNAL_RESULT_OK;
}

int main(void)
{
    peripheral_system_t system;
    const peripheral_system_config_t config = {
        .acquire_context = NULL,
        .acquire = App_Acquire,
        .algorithm_context = NULL,
        .algorithm_hook = App_AlgorithmHook,
        .output_context = NULL,
        .output = App_Output,
    };

    /* TODO hardware: call SYSCFG_DL_init() before PeripheralSystem_Init(). */
    if (PeripheralSystem_Init(&system, &config) != SIGNAL_RESULT_OK) {
        for (;;) {
        }
    }

    for (;;) {
        const signal_result_t result = PeripheralSystem_RunOnce(&system);
        if (result == SIGNAL_RESULT_NOT_SUPPORTED) {
            /* Expected until App_Acquire is connected; safe debug stop. */
            for (;;) {
            }
        }
        if (result != SIGNAL_RESULT_OK) {
            /* TODO policy: stop/re-init/report according to the contest app. */
            for (;;) {
            }
        }
    }
}
