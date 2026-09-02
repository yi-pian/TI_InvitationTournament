#include "signal_matrix_keypad_4x4.h"

#include <stddef.h>
#include <string.h>

#if defined(__MSPM0G3507__)
#include "ti_msp_dl_config.h"
#endif

static const char g_default_keymap[SIGNAL_MATRIX_KEYPAD_4X4_KEY_COUNT] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};

static signal_result_t keypad_release_all_rows(
    signal_matrix_keypad_4x4_t *keypad)
{
    uint8_t row;

    for (row = 0U; row < SIGNAL_MATRIX_KEYPAD_4X4_ROWS; ++row) {
        signal_result_t result = keypad->config.drive_row(
            keypad->config.context, row, false);
        if (result != SIGNAL_RESULT_OK) {
            return result;
        }
    }
    return SIGNAL_RESULT_OK;
}

static bool keypad_ghost_possible(uint16_t mask)
{
    uint8_t rows_with_keys = 0U;
    uint8_t columns_with_keys = 0U;
    uint8_t row;
    uint8_t column;

    for (row = 0U; row < SIGNAL_MATRIX_KEYPAD_4X4_ROWS; ++row) {
        if ((mask & (uint16_t)(UINT16_C(0x000F) << (row * 4U))) != 0U) {
            ++rows_with_keys;
        }
    }
    for (column = 0U; column < SIGNAL_MATRIX_KEYPAD_4X4_COLUMNS; ++column) {
        uint16_t column_mask = (uint16_t)(UINT16_C(0x1111) << column);
        if ((mask & column_mask) != 0U) {
            ++columns_with_keys;
        }
    }
    return (rows_with_keys > 1U) && (columns_with_keys > 1U);
}

signal_result_t SignalMatrixKeypad4x4_Init(
    signal_matrix_keypad_4x4_t *keypad,
    const signal_matrix_keypad_4x4_config_t *config)
{
    signal_result_t result;

    if ((keypad == NULL) || (config == NULL) ||
        (config->drive_row == NULL) || (config->read_column == NULL) ||
        (config->debounce_scans == 0U) ||
        ((config->settle_us != 0U) && (config->delay_us == NULL))) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    (void)memset(keypad, 0, sizeof(*keypad));
    keypad->config = *config;
    if (keypad->config.keymap == NULL) {
        keypad->config.keymap = g_default_keymap;
    }

    result = keypad_release_all_rows(keypad);
    if (result != SIGNAL_RESULT_OK) {
        return result;
    }
    keypad->initialized = true;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMatrixKeypad4x4_Scan(
    signal_matrix_keypad_4x4_t *keypad,
    signal_matrix_keypad_4x4_event_t *event)
{
    uint16_t raw_mask = 0U;
    uint16_t old_stable;
    uint8_t row;

    if ((keypad == NULL) || (event == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (!keypad->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }

    for (row = 0U; row < SIGNAL_MATRIX_KEYPAD_4X4_ROWS; ++row) {
        uint8_t column;
        signal_result_t result = keypad->config.drive_row(
            keypad->config.context, row, true);
        if (result != SIGNAL_RESULT_OK) {
            (void)keypad_release_all_rows(keypad);
            return result;
        }
        if (keypad->config.settle_us != 0U) {
            keypad->config.delay_us(keypad->config.context,
                                    keypad->config.settle_us);
        }

        for (column = 0U; column < SIGNAL_MATRIX_KEYPAD_4X4_COLUMNS;
             ++column) {
            bool active = false;
            result = keypad->config.read_column(
                keypad->config.context, column, &active);
            if (result != SIGNAL_RESULT_OK) {
                (void)keypad_release_all_rows(keypad);
                return result;
            }
            if (active) {
                uint8_t index = (uint8_t)(row * 4U + column);
                raw_mask |= (uint16_t)(UINT16_C(1) << index);
            }
        }

        result = keypad->config.drive_row(
            keypad->config.context, row, false);
        if (result != SIGNAL_RESULT_OK) {
            (void)keypad_release_all_rows(keypad);
            return result;
        }
    }

    old_stable = keypad->stable_mask;
    keypad->raw_mask = raw_mask;
    if (raw_mask == keypad->candidate_mask) {
        if (keypad->candidate_scans < keypad->config.debounce_scans) {
            ++keypad->candidate_scans;
        }
    } else {
        keypad->candidate_mask = raw_mask;
        keypad->candidate_scans = 1U;
    }
    if (keypad->candidate_scans >= keypad->config.debounce_scans) {
        keypad->stable_mask = keypad->candidate_mask;
    }

    event->raw_mask = raw_mask;
    event->stable_mask = keypad->stable_mask;
    event->pressed_mask = (uint16_t)(keypad->stable_mask &
                                     (uint16_t)~old_stable);
    event->released_mask = (uint16_t)(old_stable &
                                      (uint16_t)~keypad->stable_mask);
    event->ghost_possible = keypad_ghost_possible(raw_mask);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMatrixKeypad4x4_GetKey(
    const signal_matrix_keypad_4x4_t *keypad,
    uint8_t key_index,
    char *symbol)
{
    if ((keypad == NULL) || (symbol == NULL) ||
        (key_index >= SIGNAL_MATRIX_KEYPAD_4X4_KEY_COUNT)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (!keypad->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    *symbol = keypad->config.keymap[key_index];
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMatrixKeypad4x4_GetFirstPressed(
    const signal_matrix_keypad_4x4_t *keypad,
    char *symbol,
    uint8_t *key_index)
{
    uint8_t index;

    if ((keypad == NULL) || (symbol == NULL) || (key_index == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (!keypad->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    for (index = 0U; index < SIGNAL_MATRIX_KEYPAD_4X4_KEY_COUNT; ++index) {
        if ((keypad->stable_mask & (uint16_t)(UINT16_C(1) << index)) != 0U) {
            *symbol = keypad->config.keymap[index];
            *key_index = index;
            return SIGNAL_RESULT_OK;
        }
    }
    return SIGNAL_RESULT_NO_DATA;
}

signal_module_status_t SignalMatrixKeypad4x4_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}

#if defined(__MSPM0G3507__)
static signal_matrix_keypad_4x4_t g_mspm0g3507_keypad;
static bool g_mspm0g3507_keypad_initialized;

static signal_result_t mspm0g3507_keypad_drive_row(
    void *context, uint8_t row, bool active)
{
    uint32_t pin;

    (void)context;
    switch (row) {
    case 0U: pin = GPIO_KEYPAD_KEYPAD_R1_PIN; break;
    case 1U: pin = GPIO_KEYPAD_KEYPAD_R2_PIN; break;
    case 2U: pin = GPIO_KEYPAD_KEYPAD_R3_PIN; break;
    case 3U: pin = GPIO_KEYPAD_KEYPAD_R4_PIN; break;
    default: return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    if (active) {
        DL_GPIO_clearPins(GPIO_KEYPAD_PORT, pin);
    } else {
        DL_GPIO_setPins(GPIO_KEYPAD_PORT, pin);
    }
    return SIGNAL_RESULT_OK;
}

static signal_result_t mspm0g3507_keypad_read_column(
    void *context, uint8_t column, bool *active)
{
    uint32_t pin;

    (void)context;
    if (active == NULL) return SIGNAL_RESULT_INVALID_ARGUMENT;
    switch (column) {
    case 0U: pin = GPIO_KEYPAD_KEYPAD_C1_PIN; break;
    case 1U: pin = GPIO_KEYPAD_KEYPAD_C2_PIN; break;
    case 2U: pin = GPIO_KEYPAD_KEYPAD_C3_PIN; break;
    case 3U: pin = GPIO_KEYPAD_KEYPAD_C4_PIN; break;
    default: return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    *active = (DL_GPIO_readPins(GPIO_KEYPAD_PORT, pin) == 0U);
    return SIGNAL_RESULT_OK;
}

static void mspm0g3507_keypad_delay_us(
    void *context, uint32_t microseconds)
{
    (void)context;
    delay_cycles((CPUCLK_FREQ / 1000000U) * microseconds);
}

static const signal_matrix_keypad_4x4_config_t g_mspm0g3507_keypad_config = {
    .context = NULL,
    .drive_row = mspm0g3507_keypad_drive_row,
    .read_column = mspm0g3507_keypad_read_column,
    .delay_us = mspm0g3507_keypad_delay_us,
    .settle_us = 5U,
    .debounce_scans = 3U,
    .keymap = NULL,
};

signal_result_t SignalMatrixKeypad4x4_ReadNewSymbol(char *symbol)
{
    signal_matrix_keypad_4x4_event_t event;
    signal_result_t result;
    uint8_t key_index;

    if (symbol == NULL) return SIGNAL_RESULT_INVALID_ARGUMENT;
    if (!g_mspm0g3507_keypad_initialized) {
        result = SignalMatrixKeypad4x4_Init(
            &g_mspm0g3507_keypad, &g_mspm0g3507_keypad_config);
        if (result != SIGNAL_RESULT_OK) return result;
        g_mspm0g3507_keypad_initialized = true;
    }

    result = SignalMatrixKeypad4x4_Scan(&g_mspm0g3507_keypad, &event);
    if (result != SIGNAL_RESULT_OK) return result;
    if (event.ghost_possible || (event.pressed_mask == 0U)) {
        return SIGNAL_RESULT_NO_DATA;
    }

    for (key_index = 0U; key_index < SIGNAL_MATRIX_KEYPAD_4X4_KEY_COUNT;
         ++key_index) {
        if ((event.pressed_mask & (uint16_t)(UINT16_C(1) << key_index)) == 0U) {
            continue;
        }
        return SignalMatrixKeypad4x4_GetKey(
            &g_mspm0g3507_keypad, key_index, symbol);
    }
    return SIGNAL_RESULT_NO_DATA;
}
#endif
