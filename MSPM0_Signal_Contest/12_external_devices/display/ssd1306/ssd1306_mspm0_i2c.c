#include "ssd1306_mspm0_i2c.h"

#include "mspm0_blocking_bus.h"

#ifndef SSD1306_MSPM0_I2C_DATA_CHUNK
#define SSD1306_MSPM0_I2C_DATA_CHUNK  (7U)
#endif

bool SSD1306_MSPM0_I2C_Write(void *context,
    uint8_t control,
    const uint8_t *data,
    size_t count)
{
    ssd1306_mspm0_i2c_t *bus = (ssd1306_mspm0_i2c_t *) context;
    uint8_t packet[1U + SSD1306_MSPM0_I2C_DATA_CHUNK];
    size_t offset = 0U;

    if ((bus == NULL) || (bus->i2c == NULL) ||
        (bus->address_7bit > 0x7FU) || (data == NULL) ||
        (count == 0U) || (SSD1306_MSPM0_I2C_DATA_CHUNK == 0U) ||
        (SSD1306_MSPM0_I2C_DATA_CHUNK > 7U)) {
        return false;
    }

    packet[0] = control;
    while (offset < count) {
        size_t remaining = count - offset;
        size_t chunk = (remaining > SSD1306_MSPM0_I2C_DATA_CHUNK) ?
            SSD1306_MSPM0_I2C_DATA_CHUNK : remaining;
        size_t index;

        for (index = 0U; index < chunk; ++index) {
            packet[index + 1U] = data[offset + index];
        }
        if (!MSPM0_EXT_I2C_Write(bus->i2c, bus->address_7bit,
                packet, (uint16_t) (chunk + 1U))) {
            return false;
        }
        offset += chunk;
    }
    return true;
}
