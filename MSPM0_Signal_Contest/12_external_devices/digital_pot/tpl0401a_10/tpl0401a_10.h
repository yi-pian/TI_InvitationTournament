#ifndef MSPM0_EXTERNAL_TPL0401A_10_H
#define MSPM0_EXTERNAL_TPL0401A_10_H

#include <stdbool.h>
#include <stdint.h>
#include <ti/driverlib/driverlib.h>

#define TPL0401A_10_I2C_ADDRESS (0x2EU)
#define TPL0401B_10_I2C_ADDRESS (0x3EU)

bool TPL0401_SetWiper(I2C_Regs *i2c, uint8_t address_7bit, uint8_t position);
bool TPL0401_GetWiper(I2C_Regs *i2c, uint8_t address_7bit, uint8_t *position);

#endif
