#include "ad9850.h"

#include <stddef.h>

static uint32_t AD9850_MaxU32(uint32_t a, uint32_t b)
{
    return (a > b) ? a : b;
}

static uint32_t AD9850_CyclesToUsCeil(
    uint32_t cycles, uint32_t reference_clock_hz)
{
    uint64_t numerator = ((uint64_t) cycles * 1000000ULL) +
        (uint64_t) reference_clock_hz - 1ULL;
    uint64_t delay_us = numerator / (uint64_t) reference_clock_hz;

    return (delay_us == 0ULL) ? 1U : (uint32_t) delay_us;
}

static ad9850_status_t AD9850_WriteLine(
    ad9850_t *device, ad9850_line_t line, bool high)
{
    if (!device->config.write_line(
            device->config.io_context, line, high)) {
        return AD9850_STATUS_IO_ERROR;
    }
    return AD9850_STATUS_OK;
}

static void AD9850_DelayEdge(ad9850_t *device)
{
    uint32_t delay_us = device->config.edge_delay_us;

    device->config.delay_us(
        device->config.io_context, (delay_us == 0U) ? 1U : delay_us);
}

static ad9850_status_t AD9850_Pulse(
    ad9850_t *device, ad9850_line_t line, uint32_t high_time_us)
{
    ad9850_status_t status;

    status = AD9850_WriteLine(device, line, true);
    if (status != AD9850_STATUS_OK) {
        return status;
    }
    device->config.delay_us(
        device->config.io_context,
        AD9850_MaxU32(high_time_us, 1U));

    status = AD9850_WriteLine(device, line, false);
    if (status != AD9850_STATUS_OK) {
        return status;
    }
    AD9850_DelayEdge(device);
    return AD9850_STATUS_OK;
}

static ad9850_status_t AD9850_WriteBit(
    ad9850_t *device, bool high)
{
    ad9850_status_t status;

    status = AD9850_WriteLine(device, AD9850_LINE_DATA, high);
    if (status != AD9850_STATUS_OK) {
        return status;
    }
    AD9850_DelayEdge(device);
    return AD9850_Pulse(
        device, AD9850_LINE_W_CLK, device->config.edge_delay_us);
}

static ad9850_status_t AD9850_WriteByteLsbFirst(
    ad9850_t *device, uint8_t value)
{
    uint32_t bit_index;

    for (bit_index = 0U; bit_index < 8U; ++bit_index) {
        ad9850_status_t status = AD9850_WriteBit(
            device, ((value >> bit_index) & 1U) != 0U);
        if (status != AD9850_STATUS_OK) {
            return status;
        }
    }
    return AD9850_STATUS_OK;
}

static ad9850_status_t AD9850_WriteFrame(
    ad9850_t *device,
    uint32_t tuning_word,
    uint8_t phase_code,
    bool power_down)
{
    uint32_t byte_index;
    uint8_t control_byte = (uint8_t) ((phase_code & 0x1FU) << 3U);

    if (power_down) {
        control_byte |= 0x04U;
    }

    for (byte_index = 0U; byte_index < 4U; ++byte_index) {
        ad9850_status_t status = AD9850_WriteByteLsbFirst(
            device,
            (uint8_t) (tuning_word >> (byte_index * 8U)));
        if (status != AD9850_STATUS_OK) {
            return status;
        }
    }

    {
        ad9850_status_t status = AD9850_WriteByteLsbFirst(
            device, control_byte);
        if (status != AD9850_STATUS_OK) {
            return status;
        }
    }

    return AD9850_Pulse(
        device, AD9850_LINE_FQ_UD, device->config.edge_delay_us);
}

ad9850_status_t AD9850_ComputeTuningWord(
    uint32_t reference_clock_hz,
    uint32_t frequency_hz,
    uint32_t *tuning_word)
{
    uint64_t numerator;

    if ((reference_clock_hz == 0U) || (tuning_word == NULL)) {
        return AD9850_STATUS_BAD_ARGUMENT;
    }
    if (frequency_hz > (reference_clock_hz / 2U)) {
        return AD9850_STATUS_OUT_OF_RANGE;
    }

    numerator = ((uint64_t) frequency_hz << 32U) +
        ((uint64_t) reference_clock_hz / 2ULL);
    *tuning_word = (uint32_t)
        (numerator / (uint64_t) reference_clock_hz);
    return AD9850_STATUS_OK;
}

ad9850_status_t AD9850_Reset(ad9850_t *device)
{
    uint32_t reset_width_us;
    uint32_t recovery_us;
    ad9850_status_t status;

    if ((device == NULL) || (device->config.reference_clock_hz == 0U) ||
        (device->config.write_line == NULL) ||
        (device->config.delay_us == NULL)) {
        return AD9850_STATUS_BAD_ARGUMENT;
    }

    status = AD9850_WriteLine(device, AD9850_LINE_W_CLK, false);
    if (status != AD9850_STATUS_OK) {
        return status;
    }
    status = AD9850_WriteLine(device, AD9850_LINE_FQ_UD, false);
    if (status != AD9850_STATUS_OK) {
        return status;
    }
    status = AD9850_WriteLine(device, AD9850_LINE_DATA, false);
    if (status != AD9850_STATUS_OK) {
        return status;
    }
    status = AD9850_WriteLine(device, AD9850_LINE_RESET, false);
    if (status != AD9850_STATUS_OK) {
        return status;
    }
    AD9850_DelayEdge(device);

    reset_width_us = AD9850_CyclesToUsCeil(
        5U, device->config.reference_clock_hz);
    status = AD9850_Pulse(
        device, AD9850_LINE_RESET, reset_width_us);
    if (status != AD9850_STATUS_OK) {
        return status;
    }

    recovery_us = AD9850_CyclesToUsCeil(
        2U, device->config.reference_clock_hz);
    device->config.delay_us(device->config.io_context, recovery_us);
    device->initialized = false;
    return AD9850_STATUS_OK;
}

ad9850_status_t AD9850_Init(
    ad9850_t *device, const ad9850_config_t *config)
{
    ad9850_status_t status;

    if ((device == NULL) || (config == NULL) ||
        (config->write_line == NULL) || (config->delay_us == NULL) ||
        (config->reference_clock_hz == 0U)) {
        return AD9850_STATUS_BAD_ARGUMENT;
    }

    device->config = *config;
    device->tuning_word = 0U;
    device->phase_code = 0U;
    device->power_down = false;
    device->initialized = false;

    status = AD9850_Reset(device);
    if (status != AD9850_STATUS_OK) {
        return status;
    }

    /* The official serial-load enable sequence is RESET, one W_CLK pulse,
     * then one FQ_UD pulse. A serial-mode module/bare IC must also have the
     * required D0..D6 strap state described in the AD9850 datasheet. */
    status = AD9850_Pulse(
        device, AD9850_LINE_W_CLK, device->config.edge_delay_us);
    if (status != AD9850_STATUS_OK) {
        return status;
    }
    status = AD9850_Pulse(
        device, AD9850_LINE_FQ_UD, device->config.edge_delay_us);
    if (status != AD9850_STATUS_OK) {
        return status;
    }

    device->initialized = true;
    status = AD9850_WriteFrame(device, 0U, 0U, false);
    if (status != AD9850_STATUS_OK) {
        device->initialized = false;
        return status;
    }
    return AD9850_STATUS_OK;
}

ad9850_status_t AD9850_SetOutput(
    ad9850_t *device,
    uint32_t frequency_hz,
    uint8_t phase_code,
    bool power_down)
{
    uint32_t tuning_word;
    ad9850_status_t status;

    if (device == NULL) {
        return AD9850_STATUS_BAD_ARGUMENT;
    }
    if (!device->initialized) {
        return AD9850_STATUS_NOT_INITIALIZED;
    }
    if (phase_code > 31U) {
        return AD9850_STATUS_OUT_OF_RANGE;
    }

    status = AD9850_ComputeTuningWord(
        device->config.reference_clock_hz,
        frequency_hz,
        &tuning_word);
    if (status != AD9850_STATUS_OK) {
        return status;
    }

    status = AD9850_WriteFrame(
        device, tuning_word, phase_code, power_down);
    if (status == AD9850_STATUS_OK) {
        device->tuning_word = tuning_word;
        device->phase_code = phase_code;
        device->power_down = power_down;
    }
    return status;
}

ad9850_status_t AD9850_SetFrequencyHz(
    ad9850_t *device, uint32_t frequency_hz)
{
    if (device == NULL) {
        return AD9850_STATUS_BAD_ARGUMENT;
    }
    if (!device->initialized) {
        return AD9850_STATUS_NOT_INITIALIZED;
    }
    return AD9850_SetOutput(
        device, frequency_hz, device->phase_code, device->power_down);
}

ad9850_status_t AD9850_SetPowerDown(
    ad9850_t *device, bool power_down)
{
    ad9850_status_t status;

    if (device == NULL) {
        return AD9850_STATUS_BAD_ARGUMENT;
    }
    if (!device->initialized) {
        return AD9850_STATUS_NOT_INITIALIZED;
    }
    status = AD9850_WriteFrame(
        device, device->tuning_word, device->phase_code, power_down);
    if (status == AD9850_STATUS_OK) {
        device->power_down = power_down;
    }
    return status;
}
