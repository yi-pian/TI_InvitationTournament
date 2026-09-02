#ifndef MSPM0_EXTERNAL_BLOCKING_BUS_H
#define MSPM0_EXTERNAL_BLOCKING_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ti/driverlib/driverlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Bring-up helper only: polling, no DMA, no interrupt state machine. */
bool MSPM0_EXT_SPI_Transfer8(SPI_Regs *spi,
    GPIO_Regs *cs_port,
    uint32_t cs_pin,
    const uint8_t *tx,
    uint8_t *rx,
    size_t count);

bool MSPM0_EXT_SPI_Write16MSB(SPI_Regs *spi,
    GPIO_Regs *cs_port,
    uint32_t cs_pin,
    uint16_t word);

bool MSPM0_EXT_I2C_Write(I2C_Regs *i2c,
    uint8_t address_7bit,
    const uint8_t *data,
    uint16_t count);

bool MSPM0_EXT_I2C_Read(I2C_Regs *i2c,
    uint8_t address_7bit,
    uint8_t *data,
    uint16_t count);

bool MSPM0_EXT_I2C_WriteRead(I2C_Regs *i2c,
    uint8_t address_7bit,
    const uint8_t *tx,
    uint16_t tx_count,
    uint8_t *rx,
    uint16_t rx_count);

#ifdef __cplusplus
}
#endif

#endif
