#include "dac7811.h"

#include "mspm0_blocking_bus.h"

bool DAC7811_InitStandalone(SPI_Regs *spi,
    GPIO_Regs *sync_port,
    uint32_t sync_pin)
{
    /* C3..C0 = 1001: disable power-up daisy-chain mode. */
    return MSPM0_EXT_SPI_Write16MSB(spi, sync_port, sync_pin, 0x9000U);
}

bool DAC7811_WriteCode(SPI_Regs *spi,
    GPIO_Regs *sync_port,
    uint32_t sync_pin,
    uint16_t code_12bit)
{
    const uint16_t word = (uint16_t) (0x1000U | (code_12bit & 0x0FFFU));
    return MSPM0_EXT_SPI_Write16MSB(spi, sync_port, sync_pin, word);
}

bool DAC7811_ClearZero(SPI_Regs *spi,
    GPIO_Regs *sync_port,
    uint32_t sync_pin)
{
    return MSPM0_EXT_SPI_Write16MSB(spi, sync_port, sync_pin, 0xB000U);
}
