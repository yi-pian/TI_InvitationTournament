#include "tca6408a.h"

#include "mspm0_blocking_bus.h"

bool TCA6408A_WriteRegister(I2C_Regs *i2c, uint8_t address_7bit,
    tca6408a_register_t reg, uint8_t value)
{
    const uint8_t packet[2] = {(uint8_t) reg, value};
    return MSPM0_EXT_I2C_Write(i2c, address_7bit, packet, sizeof(packet));
}

bool TCA6408A_ReadRegister(I2C_Regs *i2c, uint8_t address_7bit,
    tca6408a_register_t reg, uint8_t *value)
{
    const uint8_t command = (uint8_t) reg;
    if (value == NULL) {
        return false;
    }
    return MSPM0_EXT_I2C_WriteRead(
        i2c, address_7bit, &command, 1U, value, 1U);
}

bool TCA6408A_SetDirection(I2C_Regs *i2c, uint8_t address_7bit,
    uint8_t input_mask)
{
    return TCA6408A_WriteRegister(i2c, address_7bit,
        TCA6408A_REG_CONFIGURATION, input_mask);
}

bool TCA6408A_WritePins(I2C_Regs *i2c, uint8_t address_7bit,
    uint8_t output_value)
{
    return TCA6408A_WriteRegister(i2c, address_7bit,
        TCA6408A_REG_OUTPUT, output_value);
}

bool TCA6408A_ReadPins(I2C_Regs *i2c, uint8_t address_7bit,
    uint8_t *input_value)
{
    return TCA6408A_ReadRegister(i2c, address_7bit,
        TCA6408A_REG_INPUT, input_value);
}
