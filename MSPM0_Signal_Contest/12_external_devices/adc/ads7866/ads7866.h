#ifndef MSPM0_EXTERNAL_ADS7866_H
#define MSPM0_EXTERNAL_ADS7866_H

#include <stdbool.h>
#include <stdint.h>
#include <ti/driverlib/driverlib.h>

bool ADS7866_ReadRaw(SPI_Regs *spi, GPIO_Regs *cs_port,
    uint32_t cs_pin, uint16_t *raw_12bit);
float ADS7866_RawToVoltage(uint16_t raw_12bit, float vdd_volts);

#endif
