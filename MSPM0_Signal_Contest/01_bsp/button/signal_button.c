#include "signal_button.h"

#include <stddef.h>
#include <string.h>

signal_result_t SignalButton_Init(
    signal_button_t *button,
    const signal_button_config_t *config)
{
    if ((button == NULL) || (config == NULL) ||
        (config->read_pressed == NULL) || (config->debounce_scans == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    (void)memset(button, 0, sizeof(*button));
    button->config = *config;
    button->initialized = true;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalButton_Update(
    signal_button_t *button,
    signal_button_event_t *event)
{
    signal_result_t result;
    bool raw_pressed;
    bool old_stable;

    if ((button == NULL) || (event == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (!button->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }

    result = button->config.read_pressed(button->config.context, &raw_pressed);
    if (result != SIGNAL_RESULT_OK) {
        return result;
    }

    old_stable = button->stable_pressed;
    button->raw_pressed = raw_pressed;
    if (raw_pressed == button->candidate_pressed) {
        if (button->candidate_scans < button->config.debounce_scans) {
            ++button->candidate_scans;
        }
    } else {
        button->candidate_pressed = raw_pressed;
        button->candidate_scans = 1U;
    }
    if (button->candidate_scans >= button->config.debounce_scans) {
        button->stable_pressed = button->candidate_pressed;
    }

    event->raw_pressed = raw_pressed;
    event->stable_pressed = button->stable_pressed;
    event->pressed = (!old_stable) && button->stable_pressed;
    event->released = old_stable && (!button->stable_pressed);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalButton_GetPressed(
    const signal_button_t *button,
    bool *pressed)
{
    if ((button == NULL) || (pressed == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (!button->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    *pressed = button->stable_pressed;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalButton_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
