/* Reuse Test 02：以完整 COPY 函数组合 04 双 ADC、40 相位和 80 TFT。 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_dual_adc_phase.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_tft_st7789_font.h"

static uint16_t adc_ch1_samples[SIGNAL_SAMPLE_COUNT];
static uint16_t adc_ch2_samples[SIGNAL_SAMPLE_COUNT];
static float sample_rate_hz;
static float phase_deg;
static float delay_s;
static const float reference_frequency_hz = 1000.0f;
static tft_st7789_t tft;
static const signal_dual_adc_config_t s_adc_config = {SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U};
static const signal_dual_adc_phase_config_t s_phase_config = {32U, 128U, 1U, 32U, 128U};

/* [COPY: 04_dual_adc_dma / AcquireDualADCFrame]
 * 同步获得两路 uint16_t ADC code；true 后同下标为同一触发时刻。 */
static bool AcquireDualADCFrame(void)
{
    if (SignalDualADC_Start(adc_ch1_samples, adc_ch2_samples,
            SIGNAL_SAMPLE_COUNT) != SIGNAL_RESULT_OK) return false;
    while (!SignalDualADC_IsFinished()) { __WFI(); }
    sample_rate_hz = (float)SignalDualADC_GetConfiguredRate();
    return true;
}

/* [COPY: 40_dual_channel_measurement / MeasurePhase + CalculateDelayFromPhase]
 * 输入同步 ADC code 和 Fs；输出 phase_deg（deg）与 delay_s（s）。 */
static bool MeasurePhaseAndDelay(void)
{
    signal_dual_adc_phase_result_t result;
    if (SignalDualADCPhase_Process(adc_ch1_samples, adc_ch2_samples,
            SIGNAL_SAMPLE_COUNT, (uint32_t)sample_rate_hz, &s_phase_config,
            &result) != SIGNAL_ALGORITHM_OK) return false;
    phase_deg = (float)result.phase_degrees;
    delay_s = phase_deg / (360.0f * reference_frequency_hz);
    return true;
}

/* [COPY: 80_tft_usage / UpdateLiveValue]
 * 局部刷新相位和延迟；数值单位分别为 deg 与 us。 */
static void UpdatePhaseDisplay(void)
{
    (void)TFT_ST7789_FillRect(&tft, 8, 8, 304, 32, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawInt32(&tft, 8, 8, (int32_t)phase_deg,
        TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawInt32(&tft, 120, 8, (int32_t)(delay_s * 1000000.0f),
        TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
}

int main(void)
{
    SYSCFG_DL_init();
    if (SignalDualADC_Init(&s_adc_config) != SIGNAL_RESULT_OK ||
        SignalTFTST7789_MSPM0_Init(&tft, TFT_ST7789_ROTATION_270, 0U, 0U) != TFT_ST7789_OK) while (true) { }
    while (true) {
        if (!AcquireDualADCFrame()) continue;
        if (!MeasurePhaseAndDelay()) continue;
        UpdatePhaseDisplay();
    }
}
