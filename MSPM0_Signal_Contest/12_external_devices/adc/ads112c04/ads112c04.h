#ifndef MSPM0_EXTERNAL_ADS112C04_H
#define MSPM0_EXTERNAL_ADS112C04_H

#include <stdbool.h>
#include <stdint.h>
#include <ti/driverlib/driverlib.h>

#define ADS112C04_ADDRESS_A1_GND_A0_GND (0x40U)

typedef enum {
    ADS112C04_REG_CONFIG0 = 0,
    ADS112C04_REG_CONFIG1 = 1,
    ADS112C04_REG_CONFIG2 = 2,
    ADS112C04_REG_CONFIG3 = 3
} ads112c04_register_t;

bool ADS112C04_SendCommand(I2C_Regs *i2c, uint8_t address_7bit,
    uint8_t command);
bool ADS112C04_WriteRegister(I2C_Regs *i2c, uint8_t address_7bit,
    ads112c04_register_t reg, uint8_t value);
bool ADS112C04_ReadRegister(I2C_Regs *i2c, uint8_t address_7bit,
    ads112c04_register_t reg, uint8_t *value);
bool ADS112C04_Reset(I2C_Regs *i2c, uint8_t address_7bit);
bool ADS112C04_StartSingle(I2C_Regs *i2c, uint8_t address_7bit);
bool ADS112C04_IsReady(I2C_Regs *i2c, uint8_t address_7bit, bool *ready);
bool ADS112C04_ReadRaw(I2C_Regs *i2c, uint8_t address_7bit, int16_t *raw);
float ADS112C04_RawToVoltage(int16_t raw, float vref_volts, uint16_t gain);

#endif
