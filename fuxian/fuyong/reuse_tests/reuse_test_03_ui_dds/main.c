/* Reuse Test 03：以完整 COPY 函数组合 70 键盘、90 DDS 与 80 TFT。 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_tft_st7789_font.h"
#include "signal_wave_output_mspm0g3507.h"

static uint16_t wave_table[256U];
static uint16_t dac_output[512U];
static float frequency_hz = 1000.0f;
static tft_st7789_t tft;
static const signal_dac_dma_mspm0_config_t s_dac_config = {SIGNAL_DAC_UPDATE_RATE_HZ, CPUCLK_FREQ, 65536U};

/* [COPY: 90_dds_usage / InitDDSOutput + SetDDSFrequency]
 * wave_table/DAC DMA 初始化完成后，用 frequency_hz（Hz）更新带偏置的正弦。 */
static bool InitDDSOutput(void)
{
    const signal_wave_output_config_t config = {wave_table, 256U, dac_output,
        512U, s_dac_config, 12U, SIGNAL_ADC_VREF_V};
    return SignalWaveOutput_Init(&config) == SIGNAL_RESULT_OK;
}

static bool SetDDSFrequency(void)
{
    return SignalWaveOutput_SineWithOffset(frequency_hz, 1.0f, 1.65f) ==
        SIGNAL_RESULT_OK;
}

/* [COPY: 70_keypad_usage / ReadKeypad + HandleParameterAdjust]
 * 仅接受星号/井号键，每次改变 10 Hz；true 表示频率已改变需要更新 DDS。 */
static bool HandleDDSKeyAdjust(void)
{
    char key;
    if (SignalMatrixKeypad4x4_ReadNewSymbol(&key) != SIGNAL_RESULT_OK) return false;
    if (key == '*' && frequency_hz > 10.0f) {
        frequency_hz -= 10.0f;
        return true;
    }
    if (key == '#') {
        frequency_hz += 10.0f;
        return true;
    }
    return false;
}

/* [COPY: 80_tft_usage / UpdateLiveValue]
 * 局部刷新当前 frequency_hz，单位 Hz。 */
static void UpdateDDSDisplay(void)
{
    (void)TFT_ST7789_FillRect(&tft, 8, 8, 160, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawInt32(&tft, 8, 8, (int32_t)frequency_hz,
        TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
}

int main(void)
{
    SYSCFG_DL_init();
    if (!InitDDSOutput() || !SetDDSFrequency() ||
        SignalTFTST7789_MSPM0_Init(&tft, TFT_ST7789_ROTATION_270, 0U, 0U) != TFT_ST7789_OK) while (true) { }
    while (true) {
        if (HandleDDSKeyAdjust()) (void)SetDDSFrequency();
        UpdateDDSDisplay();
        __WFI();
    }
}
