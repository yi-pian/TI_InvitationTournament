#include "signal_gpio.h"

#include <stddef.h>

signal_result_t SignalGPIO_Write(const signal_gpio_port_t *port, uint32_t pin,
    bool high)
{
    if ((port == NULL) || (port->write == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return port->write(port->context, pin, high);
}

signal_result_t SignalGPIO_Read(const signal_gpio_port_t *port, uint32_t pin,
    bool *high)
{
    if ((port == NULL) || (port->read == NULL) || (high == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return port->read(port->context, pin, high);
}

signal_result_t SignalGPIO_Toggle(const signal_gpio_port_t *port, uint32_t pin)
{
    if ((port == NULL) || (port->toggle == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return port->toggle(port->context, pin);
}

signal_module_status_t SignalGPIO_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
