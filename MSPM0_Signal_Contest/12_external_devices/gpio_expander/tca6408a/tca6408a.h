#ifndef MSPM0_EXTERNAL_TCA6408A_H
#define MSPM0_EXTERNAL_TCA6408A_H

#include <stdbool.h>
#include <stdint.h>
#include <ti/driverlib/driverlib.h>

#define TCA6408A_ADDRESS_ADDR_LOW  (0x20U)
#define TCA6408A_ADDRESS_ADDR_HIGH (0x21U)

typedef enum {
    TCA6408A_REG_INPUT = 0x00,
    TCA6408A_REG_OUTPUT = 0x01,
    TCA6408A_REG_POLARITY = 0x02,
    TCA6408A_REG_CONFIGURATION = 0x03
} tca6408a_register_t;

bool TCA6408A_WriteRegister(I2C_Regs *i2c, uint8_t address_7bit,
    tca6408a_register_t reg, uint8_t value);
bool TCA6408A_ReadRegister(I2C_Regs *i2c, uint8_t address_7bit,
    tca6408a_register_t reg, uint8_t *value);
bool TCA6408A_SetDirection(I2C_Regs *i2c, uint8_t address_7bit,
    uint8_t input_mask);
bool TCA6408A_WritePins(I2C_Regs *i2c, uint8_t address_7bit,
    uint8_t output_value);
bool TCA6408A_ReadPins(I2C_Regs *i2c, uint8_t address_7bit,
    uint8_t *input_value);

#endif
