#ifndef MSPM0_EXTERNAL_DAC7811_H
#define MSPM0_EXTERNAL_DAC7811_H

#include <stdbool.h>
#include <stdint.h>
#include <ti/driverlib/driverlib.h>

bool DAC7811_InitStandalone(SPI_Regs *spi,
    GPIO_Regs *sync_port,
    uint32_t sync_pin);
bool DAC7811_WriteCode(SPI_Regs *spi,
    GPIO_Regs *sync_port,
    uint32_t sync_pin,
    uint16_t code_12bit);
bool DAC7811_ClearZero(SPI_Regs *spi,
    GPIO_Regs *sync_port,
    uint32_t sync_pin);

#endif
