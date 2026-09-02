#include "signal_rotary_encoder.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

/* Index is old_AB << 2 | new_AB. Opposite-state jumps are handled separately. */
static const int8_t k_quadrature_delta[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

static signal_result_t ReadInputs(
    const signal_rotary_encoder_config_t *config,
    uint8_t *ab,
    bool *button_pressed)
{
    signal_result_t result;
    bool a_high;
    bool b_high;
    bool button_high = true;

    result = config->read_a_level(config->context, &a_high);
    if (result != SIGNAL_RESULT_OK) {
        return result;
    }
    result = config->read_b_level(config->context, &b_high);
    if (result != SIGNAL_RESULT_OK) {
        return result;
    }
    if (config->read_button_level != NULL) {
        result = config->read_button_level(config->context, &button_high);
        if (result != SIGNAL_RESULT_OK) {
            return result;
        }
    }

    *ab = (uint8_t)((a_high ? 2U : 0U) | (b_high ? 1U : 0U));
    *button_pressed = (config->read_button_level != NULL) &&
        (config->button_active_low ? !button_high : button_high);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalRotaryEncoder_Init(
    signal_rotary_encoder_t *encoder,
    const signal_rotary_encoder_config_t *config)
{
    signal_result_t result;
    uint8_t ab;
    bool button_pressed;

    if ((encoder == NULL) || (config == NULL) ||
        (config->read_a_level == NULL) || (config->read_b_level == NULL) ||
        (config->transitions_per_step == 0U) ||
        (config->transitions_per_step > 4U) ||
        ((config->read_button_level != NULL) &&
         (config->button_debounce_scans == 0U))) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    result = ReadInputs(config, &ab, &button_pressed);
    if (result != SIGNAL_RESULT_OK) {
        return result;
    }

    (void)memset(encoder, 0, sizeof(*encoder));
    encoder->config = *config;
    encoder->previous_ab = ab;
    encoder->button_candidate_pressed = button_pressed;
    encoder->button_stable_pressed = button_pressed;
    encoder->button_candidate_scans = config->button_debounce_scans;
    encoder->initialized = true;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalRotaryEncoder_Update(
    signal_rotary_encoder_t *encoder,
    signal_rotary_encoder_event_t *event)
{
    signal_result_t result;
    uint8_t current_ab;
    uint8_t transition_index;
    int8_t delta;
    bool button_pressed;
    bool old_button_stable;

    if ((encoder == NULL) || (event == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (!encoder->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }

    result = ReadInputs(&encoder->config, &current_ab, &button_pressed);
    if (result != SIGNAL_RESULT_OK) {
        return result;
    }

    (void)memset(event, 0, sizeof(*event));
    event->position = encoder->position;
    event->raw_button_pressed = button_pressed;

    if ((encoder->previous_ab ^ current_ab) == 3U) {
        event->invalid_transition = true;
        encoder->transition_accumulator = 0;
        if (encoder->invalid_transition_count < UINT32_MAX) {
            ++encoder->invalid_transition_count;
        }
    } else {
        transition_index = (uint8_t)((encoder->previous_ab << 2U) | current_ab);
        delta = k_quadrature_delta[transition_index];
        encoder->transition_accumulator =
            (int8_t)(encoder->transition_accumulator + delta);

        if (encoder->transition_accumulator >=
            (int8_t)encoder->config.transitions_per_step) {
            encoder->transition_accumulator = (int8_t)(
                encoder->transition_accumulator -
                (int8_t)encoder->config.transitions_per_step);
            if (encoder->position < INT32_MAX) {
                ++encoder->position;
                event->step_delta = 1;
            }
        } else if (encoder->transition_accumulator <=
                   -(int8_t)encoder->config.transitions_per_step) {
            encoder->transition_accumulator = (int8_t)(
                encoder->transition_accumulator +
                (int8_t)encoder->config.transitions_per_step);
            if (encoder->position > INT32_MIN) {
                --encoder->position;
                event->step_delta = -1;
            }
        }
    }
    encoder->previous_ab = current_ab;
    event->position = encoder->position;

    old_button_stable = encoder->button_stable_pressed;
    if (encoder->config.read_button_level != NULL) {
        if (button_pressed == encoder->button_candidate_pressed) {
            if (encoder->button_candidate_scans <
                encoder->config.button_debounce_scans) {
                ++encoder->button_candidate_scans;
            }
        } else {
            encoder->button_candidate_pressed = button_pressed;
            encoder->button_candidate_scans = 1U;
        }
        if (encoder->button_candidate_scans >=
            encoder->config.button_debounce_scans) {
            encoder->button_stable_pressed =
                encoder->button_candidate_pressed;
        }
    }
    event->stable_button_pressed = encoder->button_stable_pressed;
    event->button_pressed = (!old_button_stable) &&
        encoder->button_stable_pressed;
    event->button_released = old_button_stable &&
        (!encoder->button_stable_pressed);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalRotaryEncoder_GetPosition(
    const signal_rotary_encoder_t *encoder,
    int32_t *position)
{
    if ((encoder == NULL) || (position == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (!encoder->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    *position = encoder->position;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalRotaryEncoder_SetPosition(
    signal_rotary_encoder_t *encoder,
    int32_t position)
{
    if (encoder == NULL) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (!encoder->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    encoder->position = position;
    encoder->transition_accumulator = 0;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalRotaryEncoder_GetInvalidTransitionCount(
    const signal_rotary_encoder_t *encoder,
    uint32_t *count)
{
    if ((encoder == NULL) || (count == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (!encoder->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    *count = encoder->invalid_transition_count;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalRotaryEncoder_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
