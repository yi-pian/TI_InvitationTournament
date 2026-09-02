#ifndef EXTERNAL_AD9850_MSPM0_PLATFORM_H
#define EXTERNAL_AD9850_MSPM0_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include <ti/driverlib/driverlib.h>

#include "ad9850.h"

typedef struct {
    GPIO_Regs *w_clk_port;
    uint32_t w_clk_pin;
    GPIO_Regs *fq_ud_port;
    uint32_t fq_ud_pin;
    GPIO_Regs *data_port;
    uint32_t data_pin;
    GPIO_Regs *reset_port;
    uint32_t reset_pin;
    uint32_t system_clock_hz;
} ad9850_mspm0_platform_t;

bool AD9850_MSPM0_WriteLine(
    void *context, ad9850_line_t line, bool high);

void AD9850_MSPM0_DelayUs(void *context, uint32_t delay_us);

#endif /* EXTERNAL_AD9850_MSPM0_PLATFORM_H */

