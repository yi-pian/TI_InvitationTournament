#include "ad9850_mspm0_platform.h"

#include <stddef.h>

static bool AD9850_MSPM0_GetPin(
    ad9850_mspm0_platform_t *platform,
    ad9850_line_t line,
    GPIO_Regs **port,
    uint32_t *pin)
{
    switch (line) {
        case AD9850_LINE_W_CLK:
            *port = platform->w_clk_port;
            *pin = platform->w_clk_pin;
            break;
        case AD9850_LINE_FQ_UD:
            *port = platform->fq_ud_port;
            *pin = platform->fq_ud_pin;
            break;
        case AD9850_LINE_DATA:
            *port = platform->data_port;
            *pin = platform->data_pin;
            break;
        case AD9850_LINE_RESET:
            *port = platform->reset_port;
            *pin = platform->reset_pin;
            break;
        default:
            return false;
    }

    return ((*port != NULL) && (*pin != 0U));
}

bool AD9850_MSPM0_WriteLine(
    void *context, ad9850_line_t line, bool high)
{
    ad9850_mspm0_platform_t *platform =
        (ad9850_mspm0_platform_t *) context;
    GPIO_Regs *port;
    uint32_t pin;

    if ((platform == NULL) ||
        !AD9850_MSPM0_GetPin(platform, line, &port, &pin)) {
        return false;
    }

    if (high) {
        DL_GPIO_setPins(port, pin);
    } else {
        DL_GPIO_clearPins(port, pin);
    }
    return true;
}

void AD9850_MSPM0_DelayUs(void *context, uint32_t delay_us)
{
    ad9850_mspm0_platform_t *platform =
        (ad9850_mspm0_platform_t *) context;
    uint64_t cycles;

    if ((platform == NULL) || (platform->system_clock_hz == 0U) ||
        (delay_us == 0U)) {
        return;
    }

    cycles = (((uint64_t) platform->system_clock_hz * delay_us) +
        999999ULL) / 1000000ULL;
    while (cycles > 0xFFFFFFFFULL) {
        DL_Common_delayCycles(0xFFFFFFFFU);
        cycles -= 0xFFFFFFFFULL;
    }
    if (cycles != 0ULL) {
        DL_Common_delayCycles((uint32_t) cycles);
    }
}

