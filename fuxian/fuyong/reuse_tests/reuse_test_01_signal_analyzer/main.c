/* Reuse Test 01：用完整函数 COPY 组合 02、20、21、30、80；每帧 FFT 仅一次。 */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "ti_msp_dl_config.h"
#include "arm_const_structs.h"
#include "arm_math.h"
#include "signal_config.h"
#include "signal_adc_dma.h"
#include "signal_window.h"
#include "signal_window_gain_correction.h"
#include "signal_fft_parabolic_interpolation.h"
#include "signal_harmonic.h"
#include "signal_thd.h"
#include "signal_snr.h"
#include "signal_sfdr.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_tft_st7789_font.h"

#define GRAPH_X (8)
#define GRAPH_Y (72)
#define GRAPH_W (304)
#define GRAPH_H (150)

static uint16_t adc_samples[SIGNAL_SAMPLE_COUNT];
static float sample_rate_hz;
static float voltage_samples[SIGNAL_SAMPLE_COUNT];
static float centered_samples[SIGNAL_SAMPLE_COUNT];
static float fft_magnitude[(SIGNAL_SAMPLE_COUNT / 2U) + 1U];
static q15_t fft_q15[2U * SIGNAL_SAMPLE_COUNT];
static q15_t fft_magnitude_q15[SIGNAL_SAMPLE_COUNT];
static float frequency_hz, mean_v, minimum_v, maximum_v, vpp_v, rms_v, ac_rms_v;
static float thd_percent, snr_db, sfdr_db, peak_value, interpolated_bin;
static uint32_t peak_bin;
static signal_harmonic_result_t harmonics;
static tft_st7789_t tft;
static const signal_adc_dma_config_t s_adc_config = {SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U};

/* [COPY: 02_adc_dma / AcquireADCFrame] 输入 ADC，输出 adc_samples 与实际 Fs。 */
static bool AcquireADCFrame(void)
{
    if (SignalADC_Start(adc_samples, SIGNAL_SAMPLE_COUNT) != SIGNAL_RESULT_OK) return false;
    while (!SignalADC_IsFinished()) { __WFI(); }
    sample_rate_hz = (float)SignalADC_GetConfiguredTriggerRate();
    return true;
}

/* [COPY: 30_basic_measurement / ConvertADCToVoltage + MeasureBasicParameters]
 * 输出 voltage_samples、centered_samples 及 V 单位 Basic 结果。 */
static void MeasureBasicParameters(void)
{
    uint32_t index;
    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index)
        voltage_samples[index] = (float)adc_samples[index] * SIGNAL_ADC_VREF_V / 4095.0f;
    arm_mean_f32(voltage_samples, SIGNAL_SAMPLE_COUNT, &mean_v);
    arm_min_f32(voltage_samples, SIGNAL_SAMPLE_COUNT, &minimum_v, &index);
    arm_max_f32(voltage_samples, SIGNAL_SAMPLE_COUNT, &maximum_v, &index);
    vpp_v = maximum_v - minimum_v;
    arm_rms_f32(voltage_samples, SIGNAL_SAMPLE_COUNT, &rms_v);
    arm_offset_f32(voltage_samples, -mean_v, centered_samples, SIGNAL_SAMPLE_COUNT);
    arm_rms_f32(centered_samples, SIGNAL_SAMPLE_COUNT, &ac_rms_v);
}

/* [COPY: 20_fft_analysis / RunQ15FFT] 仅由 RunFFTCommon 调用。 */
static bool RunQ15FFT(const float *input)
{
    const arm_cfft_instance_q15 *instance = &arm_cfft_sR_q15_len1024;
    uint32_t index;
    float max_abs = 0.0f;
    float scale;
#if SIGNAL_SAMPLE_COUNT == 512U
    instance = &arm_cfft_sR_q15_len512;
#elif SIGNAL_SAMPLE_COUNT != 1024U
#error "Reuse Test 01 uses documented 512/1024 CMSIS Q15 FFT sizes"
#endif
    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) if (fabsf(input[index]) > max_abs) max_abs = fabsf(input[index]);
    if (max_abs == 0.0f) { arm_fill_f32(0.0f, fft_magnitude, (SIGNAL_SAMPLE_COUNT / 2U) + 1U); return true; }
    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) { fft_q15[2U * index] = (q15_t)((input[index] / max_abs) * 32767.0f); fft_q15[2U * index + 1U] = 0; }
    arm_cfft_q15(instance, fft_q15, 0U, 1U);
    arm_cmplx_mag_q15(fft_q15, fft_magnitude_q15, SIGNAL_SAMPLE_COUNT);
    /* arm_cmplx_mag_q15 输出 Q2.14，数值 1.0 对应 16384。 */
    scale = max_abs * (float)SIGNAL_SAMPLE_COUNT / 16384.0f;
    for (index = 0U; index <= SIGNAL_SAMPLE_COUNT / 2U; ++index) fft_magnitude[index] = (float)fft_magnitude_q15[index] * scale;
    return true;
}

/* [COPY: 20_fft_analysis / RunFFTCommon] 这也是该 DMA 帧唯一 arm_cfft_q15 的入口。 */
static bool RunFFTCommon(void)
{
    signal_window_result_t result;
    if (SignalWindow_Apply(centered_samples, voltage_samples, SIGNAL_SAMPLE_COUNT,
            SIGNAL_WINDOW_HANN, &result) != SIGNAL_ALGORITHM_OK) return false;
    if (!RunQ15FFT(voltage_samples)) return false;
    return SignalWindowGainCorrection_Apply(fft_magnitude, fft_magnitude,
        (SIGNAL_SAMPLE_COUNT / 2U) + 1U, SIGNAL_SAMPLE_COUNT,
        result.coherent_gain) == SIGNAL_ALGORITHM_OK;
}

/* [COPY: 20_fft_analysis / 测频、插值、谐波、THD、SNR、SFDR] 全部只读 fft_magnitude。 */
static void AnalyzeSpectrum(void)
{
    signal_fft_parabolic_result_t interpolation;
    signal_thd_result_t thd;
    signal_snr_result_t snr;
    signal_sfdr_result_t sfdr;
    arm_max_f32(&fft_magnitude[1], SIGNAL_SAMPLE_COUNT / 2U, &peak_value, &peak_bin);
    peak_bin += 1U;
    frequency_hz = (float)peak_bin * sample_rate_hz / (float)SIGNAL_SAMPLE_COUNT;
    if (SignalFFTParabolicInterpolation_Process(fft_magnitude, (SIGNAL_SAMPLE_COUNT / 2U) + 1U,
            peak_bin, sample_rate_hz, SIGNAL_SAMPLE_COUNT, &interpolation) == SIGNAL_ALGORITHM_OK) {
        interpolated_bin = interpolation.fractional_bin;
        frequency_hz = interpolation.frequency_hz;
    }
    { const signal_harmonic_config_t hc = {frequency_hz, 1U, 3U, 1U};
      const signal_snr_config_t nc = {peak_bin - 1U, peak_bin + 1U, 1U, SIGNAL_SAMPLE_COUNT / 2U, NULL, 0U};
      const signal_sfdr_config_t sc = {peak_bin - 1U, peak_bin + 1U, 1U, SIGNAL_SAMPLE_COUNT / 2U};
      if (SignalHarmonic_Process(fft_magnitude, (SIGNAL_SAMPLE_COUNT / 2U) + 1U, sample_rate_hz,
              SIGNAL_SAMPLE_COUNT, &hc, &harmonics) == SIGNAL_ALGORITHM_OK &&
          SignalTHD_Process(&harmonics, &thd) == SIGNAL_ALGORITHM_OK) thd_percent = thd.thd_percent;
      if (SignalSNR_Process(fft_magnitude, (SIGNAL_SAMPLE_COUNT / 2U) + 1U, &nc, &snr) == SIGNAL_ALGORITHM_OK) snr_db = snr.snr_db;
      if (SignalSFDR_Process(fft_magnitude, (SIGNAL_SAMPLE_COUNT / 2U) + 1U, &sc, &sfdr) == SIGNAL_ALGORITHM_OK) sfdr_db = sfdr.sfdr_db; }
}

/* [COPY: 21_time_domain_waveform / DrawTimeDomainWaveform] 使用统一 adc_samples。 */
static void DrawTimeDomainWaveform(void)
{
    uint32_t x;
    (void)TFT_ST7789_FillRect(&tft, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H, TFT_ST7789_BLACK);
    for (x = 1U; x < (uint32_t)GRAPH_W; ++x) { uint32_t a = (x - 1U) * SIGNAL_SAMPLE_COUNT / (uint32_t)GRAPH_W; uint32_t b = x * SIGNAL_SAMPLE_COUNT / (uint32_t)GRAPH_W; int32_t y0 = GRAPH_Y + GRAPH_H - 1 - (int32_t)(adc_samples[a] * (GRAPH_H - 1) / 4095U); int32_t y1 = GRAPH_Y + GRAPH_H - 1 - (int32_t)(adc_samples[b] * (GRAPH_H - 1) / 4095U); (void)TFT_ST7789_DrawLine(&tft, GRAPH_X + (int32_t)x - 1, y0, GRAPH_X + (int32_t)x, y1, TFT_ST7789_YELLOW); }
}

/* [COPY: 80_tft_usage / UpdateLiveValue] 刷新频率、Vpp、RMS、THD。 */
static void UpdateAnalyzerDisplay(void)
{
    (void)TFT_ST7789_FillRect(&tft, 8, 24, 304, 40, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawInt32(&tft, 8, 24, (int32_t)frequency_hz, TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawInt32(&tft, 88, 24, (int32_t)(vpp_v * 1000.0f), TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawInt32(&tft, 168, 24, (int32_t)(rms_v * 1000.0f), TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawInt32(&tft, 248, 24, (int32_t)thd_percent, TFT_ST7789_FONT_8X16, TFT_ST7789_RED, TFT_ST7789_BLACK, false);
}

int main(void)
{
    SYSCFG_DL_init();
    if (SignalADC_Init(&s_adc_config) != SIGNAL_RESULT_OK ||
        SignalTFTST7789_MSPM0_Init(&tft, TFT_ST7789_ROTATION_270, 0U, 0U) != TFT_ST7789_OK) while (true) { }
    (void)TFT_ST7789_FillScreen(&tft, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawString(&tft, 8, 8, "REUSE ANALYZER", TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false, false);
    while (true) {
        if (!AcquireADCFrame()) continue;
        MeasureBasicParameters();
        DrawTimeDomainWaveform();
        if (!RunFFTCommon()) continue;
        AnalyzeSpectrum();
        UpdateAnalyzerDisplay();
    }
}
