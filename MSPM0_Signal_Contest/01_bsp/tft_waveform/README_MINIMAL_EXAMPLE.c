/* 最小示例：把一段电压数组绘制到 TFT 波形区域。 */
#include "signal_tft_waveform.h"

extern tft_ili9341_t g_tft;
extern float g_voltage_v[];

void DrawCapturedFrame(size_t start_index, size_t view_count)
{
    signal_tft_waveform_result_t result;
    const signal_tft_waveform_config_t config = {
        8, 32, 304U, 160U,
        SIGNAL_TFT_WAVEFORM_MIN_MAX_ENVELOPE,
        SIGNAL_TFT_WAVEFORM_FIXED_SCALE,
        0.0F, 3.3F, 1.65F,
        TFT_ILI9341_YELLOW, TFT_ILI9341_BLACK,
        TFT_ILI9341_BLUE, TFT_ILI9341_CYAN,
        4U, 8U, true, true, true, true
    };

    /* Call after acquisition/processing, never while DMA owns this buffer. */
    (void)SignalTFTWaveform_Draw(&g_tft, &g_voltage_v[start_index],
        view_count, &config, &result);
}
