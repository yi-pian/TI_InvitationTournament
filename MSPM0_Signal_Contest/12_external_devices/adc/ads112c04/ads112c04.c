#include "ads112c04.h"

#include "mspm0_blocking_bus.h"

#define ADS112C04_CMD_RESET (0x06U)
#define ADS112C04_CMD_START (0x08U)
#define ADS112C04_CMD_RDATA (0x10U)
#define ADS112C04_CMD_RREG(reg) ((uint8_t) (0x20U | ((uint8_t) (reg) << 2U)))
#define ADS112C04_CMD_WREG(reg) ((uint8_t) (0x40U | ((uint8_t) (reg) << 2U)))

bool ADS112C04_SendCommand(I2C_Regs *i2c, uint8_t address_7bit,
    uint8_t command)
{
    return MSPM0_EXT_I2C_Write(i2c, address_7bit, &command, 1U);
}

bool ADS112C04_WriteRegister(I2C_Regs *i2c, uint8_t address_7bit,
    ads112c04_register_t reg, uint8_t value)
{
    const uint8_t packet[2] = {ADS112C04_CMD_WREG(reg), value};
    if (reg > ADS112C04_REG_CONFIG3) {
        return false;
    }
    return MSPM0_EXT_I2C_Write(i2c, address_7bit, packet, sizeof(packet));
}

bool ADS112C04_ReadRegister(I2C_Regs *i2c, uint8_t address_7bit,
    ads112c04_register_t reg, uint8_t *value)
{
    const uint8_t command = ADS112C04_CMD_RREG(reg);
    if ((reg > ADS112C04_REG_CONFIG3) || (value == NULL)) {
        return false;
    }
    return MSPM0_EXT_I2C_WriteRead(
        i2c, address_7bit, &command, 1U, value, 1U);
}

bool ADS112C04_Reset(I2C_Regs *i2c, uint8_t address_7bit)
{
    return ADS112C04_SendCommand(i2c, address_7bit, ADS112C04_CMD_RESET);
}

bool ADS112C04_StartSingle(I2C_Regs *i2c, uint8_t address_7bit)
{
    return ADS112C04_SendCommand(i2c, address_7bit, ADS112C04_CMD_START);
}

bool ADS112C04_IsReady(I2C_Regs *i2c, uint8_t address_7bit, bool *ready)
{
    uint8_t config2;
    if ((ready == NULL) || !ADS112C04_ReadRegister(i2c, address_7bit,
        ADS112C04_REG_CONFIG2, &config2)) {
        return false;
    }
    *ready = (config2 & 0x80U) != 0U;
    return true;
}

bool ADS112C04_ReadRaw(I2C_Regs *i2c, uint8_t address_7bit, int16_t *raw)
{
    uint8_t bytes[2];
    const uint8_t command = ADS112C04_CMD_RDATA;

    if ((raw == NULL) || !MSPM0_EXT_I2C_WriteRead(i2c, address_7bit,
        &command, 1U, bytes, sizeof(bytes))) {
        return false;
    }
    *raw = (int16_t) (((uint16_t) bytes[0] << 8U) | bytes[1]);
    return true;
}

float ADS112C04_RawToVoltage(int16_t raw, float vref_volts, uint16_t gain)
{
    if (gain == 0U) {
        return 0.0f;
    }
    return ((float) raw * vref_volts) / (32768.0f * (float) gain);
}
