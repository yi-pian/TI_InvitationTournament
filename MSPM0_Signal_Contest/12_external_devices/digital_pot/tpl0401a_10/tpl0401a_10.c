#include "tpl0401a_10.h"

#include "mspm0_blocking_bus.h"

bool TPL0401_SetWiper(I2C_Regs *i2c, uint8_t address_7bit, uint8_t position)
{
    const uint8_t packet[2] = {0x00U, (uint8_t) (position & 0x7FU)};
    return MSPM0_EXT_I2C_Write(i2c, address_7bit, packet, sizeof(packet));
}

bool TPL0401_GetWiper(I2C_Regs *i2c, uint8_t address_7bit, uint8_t *position)
{
    uint8_t value;

    if ((position == NULL) ||
        !MSPM0_EXT_I2C_Read(i2c, address_7bit, &value, 1U)) {
        return false;
    }
    *position = (uint8_t) (value & 0x7FU);
    return true;
}
