#ifndef EXTERNAL_AD9850_H
#define EXTERNAL_AD9850_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AD9850_STATUS_OK = 0,
    AD9850_STATUS_BAD_ARGUMENT,
    AD9850_STATUS_IO_ERROR,
    AD9850_STATUS_NOT_INITIALIZED,
    AD9850_STATUS_OUT_OF_RANGE
} ad9850_status_t;

typedef enum {
    AD9850_LINE_W_CLK = 0,
    AD9850_LINE_FQ_UD,
    AD9850_LINE_DATA,
    AD9850_LINE_RESET
} ad9850_line_t;

typedef bool (*ad9850_write_line_fn)(
    void *context, ad9850_line_t line, bool high);
typedef void (*ad9850_delay_us_fn)(void *context, uint32_t delay_us);

typedef struct {
    void *io_context;
    ad9850_write_line_fn write_line;
    ad9850_delay_us_fn delay_us;
    uint32_t reference_clock_hz;
    uint32_t edge_delay_us;
} ad9850_config_t;

typedef struct {
    ad9850_config_t config;
    uint32_t tuning_word;
    uint8_t phase_code;
    bool power_down;
    bool initialized;
} ad9850_t;

ad9850_status_t AD9850_Init(
    ad9850_t *device, const ad9850_config_t *config);

ad9850_status_t AD9850_Reset(ad9850_t *device);

ad9850_status_t AD9850_SetFrequencyHz(
    ad9850_t *device, uint32_t frequency_hz);

ad9850_status_t AD9850_SetOutput(
    ad9850_t *device,
    uint32_t frequency_hz,
    uint8_t phase_code,
    bool power_down);

ad9850_status_t AD9850_SetPowerDown(
    ad9850_t *device, bool power_down);

/*
 * Configure two initialized AD9850 devices. The second phase is
 * (phase_a_code + phase_difference_code) modulo 32. The devices should share
 * a reference clock for a stable relative phase. FQ_UD updates are sequential,
 * so this is not a hardware-synchronous start operation.
 */
ad9850_status_t AD9850_SetDualOutput(
    ad9850_t *device_a,
    ad9850_t *device_b,
    uint32_t frequency_a_hz,
    uint32_t frequency_b_hz,
    uint8_t phase_a_code,
    uint8_t phase_difference_code,
    bool power_down_a,
    bool power_down_b);

ad9850_status_t AD9850_ComputeTuningWord(
    uint32_t reference_clock_hz,
    uint32_t frequency_hz,
    uint32_t *tuning_word);

#ifdef __cplusplus
}
#endif

#endif /* EXTERNAL_AD9850_H */
