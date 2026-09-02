#ifndef SIGNAL_GPIO_H
#define SIGNAL_GPIO_H

#include <stdbool.h>
#include <stdint.h>
#include "signal_status.h"

typedef signal_result_t (*signal_gpio_write_fn)(void *context, uint32_t pin,
    bool high);
typedef signal_result_t (*signal_gpio_read_fn)(void *context, uint32_t pin,
    bool *high);
typedef signal_result_t (*signal_gpio_toggle_fn)(void *context, uint32_t pin);

typedef struct {
    void *context;
    signal_gpio_write_fn write;
    signal_gpio_read_fn read;
    signal_gpio_toggle_fn toggle;
} signal_gpio_port_t;

signal_result_t SignalGPIO_Write(const signal_gpio_port_t *port, uint32_t pin,
    bool high);
signal_result_t SignalGPIO_Read(const signal_gpio_port_t *port, uint32_t pin,
    bool *high);
signal_result_t SignalGPIO_Toggle(const signal_gpio_port_t *port, uint32_t pin);
signal_module_status_t SignalGPIO_GetModuleStatus(void);

#endif
