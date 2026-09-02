#include "signal_tft_waveform_st7789.h"

void ST7789Waveform_MinimalExample(tft_st7789_t *tft,
    const float *samples, size_t sample_count)
{
    signal_tft_waveform_st7789_result_t result;
    const signal_tft_waveform_st7789_config_t config = {
        .x = 8, .y = 32, .width = 224, .height = 160,
        .mode = SIGNAL_TFT_WAVEFORM_DECIMATE,
        .scale_mode = SIGNAL_TFT_WAVEFORM_AUTO_SCALE,
        .baseline_value = 0.0f,
        .waveform_color = TFT_ST7789_YELLOW,
        .background_color = TFT_ST7789_BLACK,
        .grid_color = TFT_ST7789_BLUE,
        .baseline_color = TFT_ST7789_CYAN,
        .horizontal_grid_divisions = 4,
        .vertical_grid_divisions = 4,
        .clear_background = true,
        .draw_grid = true,
        .draw_border = false,
        .draw_baseline = true,
    };
    (void)SignalTFTWaveformST7789_Draw(
        tft, samples, sample_count, &config, &result);
}
