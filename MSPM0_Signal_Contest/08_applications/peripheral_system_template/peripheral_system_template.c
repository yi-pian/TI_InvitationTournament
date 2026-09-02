#include "peripheral_system_template.h"

#include <stddef.h>

static signal_result_t PeripheralSystem_ValidateFrame(
    const signal_u16_frame_t *frame)
{
    if ((frame == NULL) || (frame->data == NULL) || (frame->count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if ((frame->sample_rate_hz == 0U) || (frame->adc_bits == 0U) ||
        (frame->adc_bits > 16U) || (frame->reference_voltage_v <= 0.0f)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t PeripheralSystem_Init(
    peripheral_system_t *system, const peripheral_system_config_t *config)
{
    if ((system == NULL) || (config == NULL) || (config->acquire == NULL) ||
        (config->algorithm_hook == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    system->config = *config;
    system->completed_frames = 0U;
    system->last_result = SIGNAL_RESULT_OK;
    system->initialized = true;
    return SIGNAL_RESULT_OK;
}

signal_result_t PeripheralSystem_RunOnce(peripheral_system_t *system)
{
    signal_result_t result;
    signal_u16_frame_t frame;

    if ((system == NULL) || (!system->initialized)) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }

    frame.data = NULL;
    frame.count = 0U;
    frame.sample_rate_hz = 0U;
    frame.adc_bits = 0U;
    frame.reference_voltage_v = 0.0f;

    result = system->config.acquire(system->config.acquire_context, &frame);
    if (result == SIGNAL_RESULT_OK) {
        result = PeripheralSystem_ValidateFrame(&frame);
    }
    if (result == SIGNAL_RESULT_OK) {
        result = system->config.algorithm_hook(
            system->config.algorithm_context, &frame);
    }
    if ((result == SIGNAL_RESULT_OK) && (system->config.output != NULL)) {
        result = system->config.output(system->config.output_context, &frame);
    }
    if (result == SIGNAL_RESULT_OK) {
        system->completed_frames++;
    }

    system->last_result = result;
    return result;
}
