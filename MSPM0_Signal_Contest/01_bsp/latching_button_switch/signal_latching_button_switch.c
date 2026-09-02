#include "signal_latching_button_switch.h"

#include <stddef.h>
#include <string.h>

signal_result_t SignalLatchingButtonSwitch_Init(
    signal_latching_button_switch_t *switch_module,
    const signal_latching_button_switch_config_t *config)
{
    if ((switch_module == NULL) || (config == NULL) ||
        (config->read_on == NULL) || (config->debounce_scans == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    (void)memset(switch_module, 0, sizeof(*switch_module));
    switch_module->config = *config;
    switch_module->initialized = true;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalLatchingButtonSwitch_Update(
    signal_latching_button_switch_t *switch_module,
    signal_latching_button_switch_event_t *event)
{
    signal_result_t result;
    bool raw_on;

    if ((switch_module == NULL) || (event == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (!switch_module->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }

    result = switch_module->config.read_on(
        switch_module->config.context, &raw_on);
    if (result != SIGNAL_RESULT_OK) {
        return result;
    }

    switch_module->raw_on = raw_on;
    if (raw_on == switch_module->candidate_on) {
        if (switch_module->candidate_scans <
            switch_module->config.debounce_scans) {
            ++switch_module->candidate_scans;
        }
    } else {
        switch_module->candidate_on = raw_on;
        switch_module->candidate_scans = 1U;
    }

    event->raw_on = raw_on;
    event->changed = false;
    event->turned_on = false;
    event->turned_off = false;
    if (switch_module->candidate_scans >=
        switch_module->config.debounce_scans) {
        if (!switch_module->state_valid) {
            switch_module->stable_on = switch_module->candidate_on;
            switch_module->state_valid = true;
        } else if (switch_module->stable_on != switch_module->candidate_on) {
            switch_module->stable_on = switch_module->candidate_on;
            event->changed = true;
            event->turned_on = switch_module->stable_on;
            event->turned_off = !switch_module->stable_on;
        }
    }

    event->state_valid = switch_module->state_valid;
    event->stable_on = switch_module->stable_on;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalLatchingButtonSwitch_GetState(
    const signal_latching_button_switch_t *switch_module,
    bool *on)
{
    if ((switch_module == NULL) || (on == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (!switch_module->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    if (!switch_module->state_valid) {
        return SIGNAL_RESULT_NO_DATA;
    }
    *on = switch_module->stable_on;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalLatchingButtonSwitch_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
