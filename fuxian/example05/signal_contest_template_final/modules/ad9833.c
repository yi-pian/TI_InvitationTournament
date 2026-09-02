#include "ad9833.h"

#include "mspm0_blocking_bus.h"

static bool write_word(SPI_Regs *spi, GPIO_Regs *port,
    uint32_t pin, uint16_t word)
{
    return MSPM0_EXT_SPI_Write16MSB(spi, port, pin, word);
}

bool AD9833_Init(SPI_Regs *spi, GPIO_Regs *fsync_port, uint32_t fsync_pin)
{
    /* B28=1, RESET=1: hold analog output at midscale while loading. */
    return write_word(spi, fsync_port, fsync_pin, 0x2100U);
}

bool AD9833_SetOutput(SPI_Regs *spi,
    GPIO_Regs *fsync_port,
    uint32_t fsync_pin,
    uint32_t mclk_hz,
    uint32_t output_hz,
    uint16_t phase_code_12bit,
    ad9833_wave_t wave)
{
    uint32_t tuning_word;
    uint64_t numerator;
    uint16_t control = 0x2000U; /* B28=1, FREQ0, PHASE0. */

    if ((mclk_hz == 0U) || (output_hz > (mclk_hz / 2U)) ||
        (wave > AD9833_WAVE_SQUARE)) {
        return false;
    }

    numerator = ((uint64_t) output_hz << 28U) + ((uint64_t) mclk_hz / 2U);
    tuning_word = (uint32_t) (numerator / mclk_hz) & 0x0FFFFFFFUL;

    if (!AD9833_Init(spi, fsync_port, fsync_pin) ||
        !write_word(spi, fsync_port, fsync_pin,
            (uint16_t) (0x4000U | (tuning_word & 0x3FFFU))) ||
        !write_word(spi, fsync_port, fsync_pin,
            (uint16_t) (0x4000U | ((tuning_word >> 14U) & 0x3FFFU))) ||
        !write_word(spi, fsync_port, fsync_pin,
            (uint16_t) (0xC000U | (phase_code_12bit & 0x0FFFU)))) {
        return false;
    }

    if (wave == AD9833_WAVE_TRIANGLE) {
        control |= 0x0002U;
    } else if (wave == AD9833_WAVE_SQUARE) {
        control |= 0x0028U; /* OPBITEN=1, DIV2=1, MODE=0. */
    }
    return write_word(spi, fsync_port, fsync_pin, control);
}

bool AD9833_SetDualOutput(
    const ad9833_channel_config_t *channel_a,
    const ad9833_channel_config_t *channel_b,
    uint16_t phase_a_code_12bit,
    uint16_t phase_difference_code_12bit)
{
    uint16_t phase_b_code_12bit;

    if ((channel_a == NULL) || (channel_b == NULL) ||
        (phase_a_code_12bit > 0x0FFFU) ||
        (phase_difference_code_12bit > 0x0FFFU)) {
        return false;
    }

    phase_b_code_12bit = (uint16_t) ((phase_a_code_12bit +
        phase_difference_code_12bit) & 0x0FFFU);

    if (!AD9833_SetOutput(channel_a->spi, channel_a->fsync_port,
            channel_a->fsync_pin, channel_a->mclk_hz,
            channel_a->output_hz, phase_a_code_12bit, channel_a->wave)) {
        return false;
    }
    return AD9833_SetOutput(channel_b->spi, channel_b->fsync_port,
        channel_b->fsync_pin, channel_b->mclk_hz,
        channel_b->output_hz, phase_b_code_12bit, channel_b->wave);
}
