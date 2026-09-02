#include "ads7887.h"

#include "mspm0_blocking_bus.h"

bool ADS7887_ReadRaw(SPI_Regs *spi, GPIO_Regs *cs_port,
    uint32_t cs_pin, uint16_t *raw_10bit)
{
    uint8_t rx[2];
    uint16_t frame;

    if ((raw_10bit == NULL) || !MSPM0_EXT_SPI_Transfer8(
        spi, cs_port, cs_pin, NULL, rx, sizeof(rx))) {
        return false;
    }
    frame = (uint16_t) (((uint16_t) rx[0] << 8U) | rx[1]);
    *raw_10bit = (uint16_t) ((frame >> 2U) & 0x03FFU);
    return true;
}

float ADS7887_RawToVoltage(uint16_t raw_10bit, float vdd_volts)
{
    return ((float) (raw_10bit & 0x03FFU) * vdd_volts) / 1024.0f;
}
