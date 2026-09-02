#ifndef MSPM0_EXTERNAL_SSD1306_MSPM0_I2C_H
#define MSPM0_EXTERNAL_SSD1306_MSPM0_I2C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ti/driverlib/driverlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    I2C_Regs *i2c;
    uint8_t address_7bit;
} ssd1306_mspm0_i2c_t;

bool SSD1306_MSPM0_I2C_Write(void *context,
    uint8_t control,
    const uint8_t *data,
    size_t count);

#ifdef __cplusplus
}
#endif

#endif /* MSPM0_EXTERNAL_SSD1306_MSPM0_I2C_H */
