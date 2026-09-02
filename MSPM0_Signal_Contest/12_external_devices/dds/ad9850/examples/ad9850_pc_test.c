#include "ad9850.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    bool lines[4];
    bool shifted_bits[64];
    uint32_t shifted_count;
} mock_io_t;

static bool Mock_WriteLine(
    void *context, ad9850_line_t line, bool high)
{
    mock_io_t *io = (mock_io_t *) context;

    if ((line < AD9850_LINE_W_CLK) || (line > AD9850_LINE_RESET)) {
        return false;
    }
    if ((line == AD9850_LINE_W_CLK) && high &&
        (io->shifted_count < 64U)) {
        io->shifted_bits[io->shifted_count++] =
            io->lines[AD9850_LINE_DATA];
    }
    io->lines[line] = high;
    return true;
}

static void Mock_DelayUs(void *context, uint32_t delay_us)
{
    (void) context;
    (void) delay_us;
}

static uint32_t BitsToU32(const bool *bits)
{
    uint32_t value = 0U;
    uint32_t index;

    for (index = 0U; index < 32U; ++index) {
        if (bits[index]) {
            value |= (1UL << index);
        }
    }
    return value;
}

static uint8_t BitsToU8(const bool *bits)
{
    uint8_t value = 0U;
    uint32_t index;

    for (index = 0U; index < 8U; ++index) {
        if (bits[index]) {
            value = (uint8_t) (value | (uint8_t) (1U << index));
        }
    }
    return value;
}

int main(void)
{
    mock_io_t io;
    mock_io_t io_b;
    ad9850_t device;
    ad9850_t device_b;
    ad9850_t uninitialized_device;
    ad9850_config_t config;
    uint32_t tuning_word;

    memset(&io, 0, sizeof(io));
    memset(&io_b, 0, sizeof(io_b));
    memset(&device, 0, sizeof(device));
    memset(&device_b, 0, sizeof(device_b));
    memset(&uninitialized_device, 0, sizeof(uninitialized_device));
    config.io_context = &io;
    config.write_line = Mock_WriteLine;
    config.delay_us = Mock_DelayUs;
    config.reference_clock_hz = 125000000U;
    config.edge_delay_us = 1U;

    assert(AD9850_ComputeTuningWord(
        125000000U, 1000000U, &tuning_word) == AD9850_STATUS_OK);
    assert(tuning_word == 0x020C49BAUL);
    assert(AD9850_SetFrequencyHz(&uninitialized_device, 1000U) ==
        AD9850_STATUS_NOT_INITIALIZED);
    assert(AD9850_Init(&device, &config) == AD9850_STATUS_OK);

    config.io_context = &io_b;
    assert(AD9850_Init(&device_b, &config) == AD9850_STATUS_OK);
    config.io_context = &io;

    io.shifted_count = 0U;
    memset(io.shifted_bits, 0, sizeof(io.shifted_bits));
    assert(AD9850_SetOutput(
        &device, 1000000U, 3U, false) == AD9850_STATUS_OK);
    assert(io.shifted_count == 40U);
    assert(BitsToU32(io.shifted_bits) == 0x020C49BAUL);
    assert(BitsToU8(&io.shifted_bits[32]) == 0x18U);
    assert(AD9850_SetFrequencyHz(
        &device, 62500001U) == AD9850_STATUS_OUT_OF_RANGE);
    assert(AD9850_SetDualOutput(
        &device, &device_b, 1000000U, 2000000U, 1U, 8U,
        false, false) == AD9850_STATUS_OK);
    assert(device.phase_code == 1U);
    assert(device_b.phase_code == 9U);
    assert(AD9850_SetDualOutput(
        &device, &device_b, 1000000U, 2000000U, 1U, 32U,
        false, false) == AD9850_STATUS_BAD_ARGUMENT);

    puts("AD9850 PC test: PASS");
    return 0;
}
