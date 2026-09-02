#ifndef SIGNAL_UART_H
#define SIGNAL_UART_H

#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

typedef signal_result_t (*signal_uart_write_fn)(void *context,
    const uint8_t *data, size_t count);
typedef signal_result_t (*signal_uart_read_fn)(void *context, uint8_t *data,
    size_t capacity, size_t *received);

typedef struct {
    void *context;
    signal_uart_write_fn write;
    signal_uart_read_fn read;
    uint32_t baud_rate;
} signal_uart_t;

signal_result_t SignalUART_Write(const signal_uart_t *uart,
    const uint8_t *data, size_t count);
signal_result_t SignalUART_WriteString(const signal_uart_t *uart,
    const char *text);
signal_result_t SignalUART_Read(const signal_uart_t *uart, uint8_t *data,
    size_t capacity, size_t *received);
signal_module_status_t SignalUART_GetModuleStatus(void);

#endif
