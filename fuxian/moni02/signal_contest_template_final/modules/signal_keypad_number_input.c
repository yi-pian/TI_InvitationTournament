#include "signal_keypad_number_input.h"

#include <stddef.h>
#include <string.h>

static void SignalKeypadNumberInput_Clear(
    signal_keypad_number_input_t *input)
{
    input->text[0] = '\0';
    input->length = 0U;
    input->decimal_present = false;
}

static signal_result_t SignalKeypadNumberInput_Parse(
    const signal_keypad_number_input_t *input, float *value)
{
    float parsed_value = 0.0f;
    float decimal_scale = 0.1f;
    bool after_decimal = false;
    uint8_t index;

    if ((input == NULL) || (value == NULL) || (input->length == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    for (index = 0U; index < input->length; ++index) {
        char character = input->text[index];

        if (character == '.') {
            after_decimal = true;
        } else if ((character >= '0') && (character <= '9')) {
            float digit = (float)(character - '0');

            if (after_decimal) {
                parsed_value += digit * decimal_scale;
                decimal_scale *= 0.1f;
            } else {
                parsed_value = parsed_value * 10.0f + digit;
            }
        } else {
            return SIGNAL_RESULT_NUMERIC_ERROR;
        }
    }

    *value = parsed_value;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalKeypadNumberInput_Init(
    signal_keypad_number_input_t *input)
{
    if (input == NULL) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }

    (void)memset(input, 0, sizeof(*input));
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalKeypadNumberInput_HandleKey(
    signal_keypad_number_input_t *input, char key,
    signal_keypad_number_input_event_t *event)
{
    signal_result_t result;

    if ((input == NULL) || (event == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *event = SIGNAL_KEYPAD_NUMBER_INPUT_NO_EVENT;

    if ((key >= '0') && (key <= '9')) {
        if (!input->active) {
            SignalKeypadNumberInput_Clear(input);
            input->active = true;
            input->has_confirmed_value = false;
        }
        if (input->length >= SIGNAL_KEYPAD_NUMBER_INPUT_CAPACITY) {
            return SIGNAL_RESULT_INSUFFICIENT_BUFFER;
        }
        input->text[input->length++] = key;
        input->text[input->length] = '\0';
        *event = SIGNAL_KEYPAD_NUMBER_INPUT_UPDATED;
        return SIGNAL_RESULT_OK;
    }

    if (key == '*') {
        if (!input->active) {
            SignalKeypadNumberInput_Clear(input);
            input->active = true;
            input->has_confirmed_value = false;
        }
        if (input->decimal_present) {
            return SIGNAL_RESULT_NO_DATA;
        }
        if (input->length == 0U) {
            if (SIGNAL_KEYPAD_NUMBER_INPUT_CAPACITY < 2U) {
                return SIGNAL_RESULT_INSUFFICIENT_BUFFER;
            }
            input->text[input->length++] = '0';
        }
        if (input->length >= SIGNAL_KEYPAD_NUMBER_INPUT_CAPACITY) {
            return SIGNAL_RESULT_INSUFFICIENT_BUFFER;
        }
        input->text[input->length++] = '.';
        input->text[input->length] = '\0';
        input->decimal_present = true;
        *event = SIGNAL_KEYPAD_NUMBER_INPUT_UPDATED;
        return SIGNAL_RESULT_OK;
    }

    if (input->active && (key == 'D')) {
        char removed_character;

        if (input->length == 0U) {
            return SIGNAL_RESULT_NO_DATA;
        }
        removed_character = input->text[--input->length];
        input->text[input->length] = '\0';
        if (removed_character == '.') {
            input->decimal_present = false;
        }
        *event = SIGNAL_KEYPAD_NUMBER_INPUT_UPDATED;
        return SIGNAL_RESULT_OK;
    }

    if (input->active && (key == 'C')) {
        SignalKeypadNumberInput_Clear(input);
        input->active = false;
        input->has_confirmed_value = false;
        *event = SIGNAL_KEYPAD_NUMBER_INPUT_CANCELLED;
        return SIGNAL_RESULT_OK;
    }

    if (input->active && (key == '#')) {
        result = SignalKeypadNumberInput_Parse(
            input, &input->confirmed_value);
        if (result != SIGNAL_RESULT_OK) {
            return result;
        }
        input->active = false;
        input->has_confirmed_value = true;
        *event = SIGNAL_KEYPAD_NUMBER_INPUT_CONFIRMED;
        return SIGNAL_RESULT_OK;
    }

    return SIGNAL_RESULT_NO_DATA;
}

signal_result_t SignalKeypadNumberInput_GetValue(
    const signal_keypad_number_input_t *input, float *value)
{
    if ((input == NULL) || (value == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (!input->has_confirmed_value) {
        return SIGNAL_RESULT_NO_DATA;
    }
    *value = input->confirmed_value;
    return SIGNAL_RESULT_OK;
}

const char *SignalKeypadNumberInput_GetText(
    const signal_keypad_number_input_t *input)
{
    static const char empty_text[] = "";

    return (input == NULL) ? empty_text : input->text;
}

bool SignalKeypadNumberInput_IsActive(
    const signal_keypad_number_input_t *input)
{
    return (input != NULL) && input->active;
}

signal_module_status_t SignalKeypadNumberInput_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
