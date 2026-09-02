#include "ads7866.h"

#include "mspm0_blocking_bus.h"

bool ADS7866_ReadRaw(SPI_Regs *spi, GPIO_Regs *cs_port,
    uint32_t cs_pin, uint16_t *raw_12bit)
{
    uint8_t rx[2];
    uint16_t frame;

    if ((raw_12bit == NULL) || !MSPM0_EXT_SPI_Transfer8(
        spi, cs_port, cs_pin, NULL, rx, sizeof(rx))) {
        return false;
    }
    frame = (uint16_t) (((uint16_t) rx[0] << 8U) | rx[1]);
    *raw_12bit = (uint16_t) (frame & 0x0FFFU);
    return true;
}

float ADS7866_RawToVoltage(uint16_t raw_12bit, float vdd_volts)
{
    return ((float) (raw_12bit & 0x0FFFU) * vdd_volts) / 4096.0f;
}
