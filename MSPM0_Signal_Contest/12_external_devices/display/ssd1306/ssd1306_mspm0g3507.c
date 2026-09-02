#include "ssd1306_mspm0g3507.h"

#include "ti_msp_dl_config.h"

ssd1306_status_t SignalSSD1306_MSPM0_Init(ssd1306_t *device,
    ssd1306_mspm0_i2c_t *bus, uint8_t address_7bit, bool rotate_180)
{
    const ssd1306_config_t config = {
        .io_context = bus,
        .write = SSD1306_MSPM0_I2C_Write,
        .contrast = 0xCFU,
        .rotate_180 = rotate_180
    };
    if (bus == NULL) return SSD1306_STATUS_BAD_ARGUMENT;
    bus->i2c = I2C_INST;
    bus->address_7bit = address_7bit;
    return SSD1306_Init(device, &config);
}
