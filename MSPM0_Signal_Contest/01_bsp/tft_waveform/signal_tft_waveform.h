#ifndef SIGNAL_TFT_WAVEFORM_H
#define SIGNAL_TFT_WAVEFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "signal_status.h"
#include "signal_tft_ili9341.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SIGNAL_TFT_WAVEFORM_DECIMATE = 0,
    SIGNAL_TFT_WAVEFORM_MIN_MAX_ENVELOPE
} signal_tft_waveform_mode_t;

typedef enum {
    SIGNAL_TFT_WAVEFORM_FIXED_SCALE = 0,
    SIGNAL_TFT_WAVEFORM_AUTO_SCALE
} signal_tft_waveform_scale_mode_t;

typedef struct {
    int32_t x;
    int32_t y;
    uint16_t width;
    uint16_t height;
    signal_tft_waveform_mode_t mode;
    signal_tft_waveform_scale_mode_t scale_mode;
    float minimum_value; /* Used by fixed scale. */
    float maximum_value; /* Used by fixed scale. */
    float baseline_value;
    uint16_t waveform_color;
    uint16_t background_color;
    uint16_t grid_color;
    uint16_t baseline_color;
    uint8_t horizontal_grid_divisions;
    uint8_t vertical_grid_divisions;
    bool clear_background;
    bool draw_grid;
    bool draw_border;
    bool draw_baseline;
} signal_tft_waveform_config_t;

typedef struct {
    float data_minimum;
    float data_maximum;
    float scale_minimum;
    float scale_maximum;
    uint16_t plotted_columns;
} signal_tft_waveform_result_t;

/* Map one finite value to a clipped Y coordinate inside the plot rectangle. */
signal_result_t SignalTFTWaveform_MapY(
    float value,
    float scale_minimum,
    float scale_maximum,
    int32_t plot_y,
    uint16_t plot_height,
    int32_t *screen_y);

/* Return the min/max represented by one equal-width display column. */
signal_result_t SignalTFTWaveform_GetEnvelopeColumn(
    const float *samples,
    size_t sample_count,
    uint16_t column_count,
    uint16_t column,
    float *minimum,
    float *maximum);

/* Caller passes the already selected/trigger-aligned view of the waveform. */
signal_result_t SignalTFTWaveform_Draw(
    tft_ili9341_t *tft,
    const float *samples,
    size_t sample_count,
    const signal_tft_waveform_config_t *config,
    signal_tft_waveform_result_t *result);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */

signal_module_status_t SignalTFTWaveform_GetModuleStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_TFT_WAVEFORM_H */

