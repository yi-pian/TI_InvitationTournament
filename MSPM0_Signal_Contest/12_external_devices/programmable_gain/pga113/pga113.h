#ifndef MSPM0_EXTERNAL_PGA113_H
#define MSPM0_EXTERNAL_PGA113_H

#include <stdbool.h>
#include <stdint.h>
#include <ti/driverlib/driverlib.h>

typedef enum {
    PGA113_GAIN_1 = 0,
    PGA113_GAIN_2,
    PGA113_GAIN_5,
    PGA113_GAIN_10,
    PGA113_GAIN_20,
    PGA113_GAIN_50,
    PGA113_GAIN_100,
    PGA113_GAIN_200
} pga113_gain_t;

typedef enum {
    PGA113_CHANNEL_0 = 0,
    PGA113_CHANNEL_1 = 1
} pga113_channel_t;

bool PGA113_SetGainAndChannel(SPI_Regs *spi,
    GPIO_Regs *cs_port,
    uint32_t cs_pin,
    pga113_gain_t gain,
    pga113_channel_t channel);

bool PGA113_Shutdown(SPI_Regs *spi, GPIO_Regs *cs_port, uint32_t cs_pin);
bool PGA113_Wakeup(SPI_Regs *spi, GPIO_Regs *cs_port, uint32_t cs_pin);

#endif
