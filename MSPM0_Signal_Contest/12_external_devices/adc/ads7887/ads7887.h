#ifndef MSPM0_EXTERNAL_ADS7887_H
#define MSPM0_EXTERNAL_ADS7887_H

#include <stdbool.h>
#include <stdint.h>
#include <ti/driverlib/driverlib.h>

bool ADS7887_ReadRaw(SPI_Regs *spi, GPIO_Regs *cs_port,
    uint32_t cs_pin, uint16_t *raw_10bit);
float ADS7887_RawToVoltage(uint16_t raw_10bit, float vdd_volts);

#endif
