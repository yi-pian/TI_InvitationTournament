#ifndef MSPM0_EXTERNAL_AD9833_H
#define MSPM0_EXTERNAL_AD9833_H

#include <stdbool.h>
#include <stdint.h>
#include <ti/driverlib/driverlib.h>

typedef enum {
    AD9833_WAVE_SINE = 0,
    AD9833_WAVE_TRIANGLE,
    AD9833_WAVE_SQUARE
} ad9833_wave_t;

typedef struct {
    SPI_Regs *spi;
    GPIO_Regs *fsync_port;
    uint32_t fsync_pin;
    uint32_t mclk_hz;
    uint32_t output_hz;
    ad9833_wave_t wave;
} ad9833_channel_config_t;

bool AD9833_Init(SPI_Regs *spi, GPIO_Regs *fsync_port, uint32_t fsync_pin);
bool AD9833_SetOutput(SPI_Regs *spi,
    GPIO_Regs *fsync_port,
    uint32_t fsync_pin,
    uint32_t mclk_hz,
    uint32_t output_hz,
    uint16_t phase_code_12bit,
    ad9833_wave_t wave);

/*
 * Configure two AD9833 devices. The second phase is
 * (phase_a_code_12bit + phase_difference_code_12bit) modulo 4096.
 * Both devices should use a common MCLK when a stable relative phase is
 * required. Bus writes are sequential, so this is not a hardware-synchronous
 * start operation.
 */
bool AD9833_SetDualOutput(
    const ad9833_channel_config_t *channel_a,
    const ad9833_channel_config_t *channel_b,
    uint16_t phase_a_code_12bit,
    uint16_t phase_difference_code_12bit);

#endif
