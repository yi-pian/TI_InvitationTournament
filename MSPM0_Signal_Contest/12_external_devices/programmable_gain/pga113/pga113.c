#include "pga113.h"

#include "mspm0_blocking_bus.h"

bool PGA113_SetGainAndChannel(SPI_Regs *spi,
    GPIO_Regs *cs_port,
    uint32_t cs_pin,
    pga113_gain_t gain,
    pga113_channel_t channel)
{
    uint16_t command;

    if ((gain > PGA113_GAIN_200) || (channel > PGA113_CHANNEL_1)) {
        return false;
    }
    command = (uint16_t) (0x2A00U |
        ((uint16_t) gain << 4U) | (uint16_t) channel);
    return MSPM0_EXT_SPI_Write16MSB(spi, cs_port, cs_pin, command);
}

bool PGA113_Shutdown(SPI_Regs *spi, GPIO_Regs *cs_port, uint32_t cs_pin)
{
    return MSPM0_EXT_SPI_Write16MSB(spi, cs_port, cs_pin, 0xE1F1U);
}

bool PGA113_Wakeup(SPI_Regs *spi, GPIO_Regs *cs_port, uint32_t cs_pin)
{
    return MSPM0_EXT_SPI_Write16MSB(spi, cs_port, cs_pin, 0xE100U);
}
