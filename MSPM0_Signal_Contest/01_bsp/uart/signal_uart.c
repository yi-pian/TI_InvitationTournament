#include "signal_uart.h"

#include <stddef.h>
#include <string.h>

signal_result_t SignalUART_Write(const signal_uart_t *uart,
    const uint8_t *data, size_t count)
{
    if ((uart == NULL) || (uart->write == NULL) ||
        ((data == NULL) && (count != 0U))) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return uart->write(uart->context, data, count);
}

signal_result_t SignalUART_WriteString(const signal_uart_t *uart,
    const char *text)
{
    if (text == NULL) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return SignalUART_Write(uart, (const uint8_t *) text, strlen(text));
}

signal_result_t SignalUART_Read(const signal_uart_t *uart, uint8_t *data,
    size_t capacity, size_t *received)
{
    if ((uart == NULL) || (uart->read == NULL) || (data == NULL) ||
        (capacity == 0U) || (received == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return uart->read(uart->context, data, capacity, received);
}

signal_module_status_t SignalUART_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
