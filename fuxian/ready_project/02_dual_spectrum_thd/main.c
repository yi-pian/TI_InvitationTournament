/* ============================================================
 * 工程：02_dual_spectrum_thd
 * 用途：双通道频谱/谐波/THD/SNR/SFDR 分析仪。
 * 输入：CH1=PA25、CH2=PA17；输出：ST7789；键盘：
 * A/B 上/下页，C 切换频谱横轴范围（5/10/25/50 kHz）。
 * 页面：CH1 spectrum、CH2 spectrum、SUMMARY。
 *
 * 来源：04_dual_adc_dma、20_fft_analysis、22_spectrum_display、70、80；
 * 平台闭包来自 example04。每通道每帧 FFT 恰好一次；THD/SNR/SFDR/绘图
 * 只复用 fft_ch1_magnitude/fft_ch2_magnitude，绝不重新 FFT。
 * 经授权复制 modules/.syscfg，未修改模块或生成文件。
 * ============================================================ */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include "arm_const_structs.h"
#include "arm_math.h"
#include "ti_msp_dl_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_fft_parabolic_interpolation.h"
#include "signal_harmonic.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_sfdr.h"
#include "signal_snr.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_thd.h"
#include "signal_window.h"
#include "signal_window_gain_correction.h"

#define SAMPLE_COUNT            (512U)
#define SAMPLE_RATE_REQUEST_HZ  (100000U)
#define ADC_REFERENCE_V         (3.3f)
#define GRAPH_X                 (8)
#define GRAPH_Y                 (64)
#define GRAPH_W                 (304)
#define GRAPH_H                 (136)
#define KEYPAD_SCAN_MS          (5U)
#define KEY_QUEUE_SIZE          (8U)
#define DISPLAY_PERIOD_MS       (350U)

typedef enum { PAGE_CH1_SPECTRUM = 0U, PAGE_CH2_SPECTRUM,
    PAGE_SUMMARY, PAGE_COUNT } app_page_t;
typedef struct {
    float frequency_hz;
    float thd_percent;
    float snr_db;
    float sfdr_db;
    float harmonic_percent[3U];
    uint32_t peak_bin;
    float peak_value;
    bool valid;
} channel_analysis_t;

static const float spectrum_range_hz[] = {5000.0f, 10000.0f, 25000.0f, 50000.0f};
static uint16_t adc_ch1_samples[SAMPLE_COUNT];
static uint16_t adc_ch2_samples[SAMPLE_COUNT];
static float voltage_ch1_samples[SAMPLE_COUNT];
static float voltage_ch2_samples[SAMPLE_COUNT];
static float centered_ch1_samples[SAMPLE_COUNT];
static float centered_ch2_samples[SAMPLE_COUNT];
static float fft_input[SAMPLE_COUNT];
static float fft_ch1_magnitude[SAMPLE_COUNT / 2U + 1U];
static float fft_ch2_magnitude[SAMPLE_COUNT / 2U + 1U];
static q15_t fft_q15[2U * SAMPLE_COUNT];
static q15_t fft_magnitude_q15[SAMPLE_COUNT];
static float sample_rate_hz = (float)SAMPLE_RATE_REQUEST_HZ;
static channel_analysis_t ch1_analysis, ch2_analysis;
static app_page_t current_page = PAGE_CH1_SPECTRUM;
static app_page_t displayed_page = PAGE_COUNT;
static uint8_t spectrum_range_index = 3U;
static tft_st7789_t tft;
static volatile char key_queue[KEY_QUEUE_SIZE];
static volatile uint8_t key_queue_head;
static volatile uint8_t key_queue_tail;
static volatile uint16_t display_elapsed_ms;
static volatile bool display_due = true;

static void DrawText(int32_t x, int32_t y, const char *text, uint16_t color)
{
    (void)TFT_ST7789_DrawString(&tft, x, y, text, TFT_ST7789_FONT_8X16,
        color, TFT_ST7789_BLACK, false, false);
}

/* ============================================================
 * [函数] AcquireDualADCFrame
 * [功能] 同一 Timer/Event 下取得同步 CH1/CH2 帧。
 * [来源] [FUYONG_COPY] 04_dual_adc_dma/DUAL_ADC_DMA。
 * [输入] 两路模拟信号；[输出] adc_ch1/adc_ch2；[单位] code。
 * [全局] 原始数组/Fs；[步骤] Start→等待双 DMA→读实际率。
 * [原因] 两通道结果必须属于同一帧；[单帧唯一] 是。
 * [复用] 两路后续链；[差异] 统一变量名；[依赖] dual_adc。
 * ============================================================ */
static bool AcquireDualADCFrame(void)
{
    if (SignalDualADC_Start(adc_ch1_samples, adc_ch2_samples,
            SAMPLE_COUNT) != SIGNAL_RESULT_OK) return false;
    while (!SignalDualADC_IsFinished()) __WFI();
    sample_rate_hz = (float)SignalDualADC_GetConfiguredRate();
    return sample_rate_hz > 0.0f;
}

/* ============================================================
 * [函数] PrepareSignal
 * [功能] 任一路唯一一次 code→V、mean 和去 DC。
 * [来源] [FUYONG_ADAPTED] 20_fft_analysis/PrepareSignal。
 * [输入] raw/count；[输出] voltage/centered；[单位] code→V。
 * [全局] 无；[步骤] 换算→arm_mean→arm_offset。
 * [原因] DC 会占满 bin0；[单帧唯一] 每通道一次。
 * [复用] 本通道 FFT；[差异] 参数化，算法不变；[依赖] CMSIS。
 * ============================================================ */
static void PrepareSignal(const uint16_t *raw, float *voltage,
    float *centered, uint32_t count)
{
    uint32_t index;
    float mean_v;
    for (index = 0U; index < count; ++index)
        voltage[index] = (float)raw[index] * ADC_REFERENCE_V / 4095.0f;
    arm_mean_f32(voltage, count, &mean_v);
    arm_offset_f32(voltage, -mean_v, centered, count);
}

/* ============================================================
 * [函数] RunFFTCommon
 * [功能] 对任一路 centered 数据执行唯一一次 Hann+Q15 FFT。
 * [来源] [FUYONG_ADAPTED] 20/RunQ15FFT+RunFFTCommon。
 * [输入] centered_input；[输出] magnitude_output；[单位] V/bin。
 * [全局] 共用 fft_input/fft_q15/q15 magnitude。
 * [步骤] 窗→归一化→CFFT→幅值→恢复→coherent gain 校正。
 * [原因] CH1/CH2 顺序复用工作区，避免双份 RAM。
 * [单帧唯一] 每通道一次；[复用] 频率/谐波/THD/SNR/SFDR/绘图。
 * [差异] 仅输入输出参数化；[依赖] CMSIS/window/gain correction。
 * ============================================================ */
static bool RunFFTCommon(const float *centered_input, float *magnitude_output)
{
    uint32_t index;
    float max_abs = 0.0f, restore_scale;
    signal_window_result_t window_result;
    if (SignalWindow_Apply(centered_input, fft_input, SAMPLE_COUNT,
            SIGNAL_WINDOW_HANN, &window_result) != SIGNAL_ALGORITHM_OK) return false;
    for (index = 0U; index < SAMPLE_COUNT; ++index) {
        const float a = fabsf(fft_input[index]);
        if (a > max_abs) max_abs = a;
    }
    if (max_abs == 0.0f) return false;
    for (index = 0U; index < SAMPLE_COUNT; ++index) {
        fft_q15[2U * index] = (q15_t)(fft_input[index] / max_abs * 32767.0f);
        fft_q15[2U * index + 1U] = 0;
    }
    arm_cfft_q15(&arm_cfft_sR_q15_len512, fft_q15, 0U, 1U);
    arm_cmplx_mag_q15(fft_q15, fft_magnitude_q15, SAMPLE_COUNT);
    restore_scale = max_abs * (float)SAMPLE_COUNT / 16384.0f;
    for (index = 0U; index <= SAMPLE_COUNT / 2U; ++index)
        magnitude_output[index] = (float)fft_magnitude_q15[index] * restore_scale;
    return SignalWindowGainCorrection_Apply(magnitude_output, magnitude_output,
        SAMPLE_COUNT / 2U + 1U, SAMPLE_COUNT,
        window_result.coherent_gain) == SIGNAL_ALGORITHM_OK;
}

/* ============================================================
 * [函数] AnalyzeChannelSpectrum
 * [功能] 从已存在的一路 magnitude 提取插值频率、H2/H3、THD、SNR、SFDR。
 * [来源] [FUYONG_ADAPTED] 20 的 Frequency/Interpolation/Harmonics/THD/SNR/SFDR。
 * [输入] magnitude；[输出] channel_analysis_t；[单位] Hz/%/dB。
 * [全局] sample_rate_hz；[步骤] 找峰→插值→谐波→THD→SNR/SFDR。
 * [原因] 只读 FFT 结果，本函数绝不调用 FFT。
 * [单帧唯一] 每通道一次；[复用] 频谱和汇总页。
 * [差异] 结果参数化并保存两路；[依赖] analysis modules。
 * ============================================================ */
static bool AnalyzeChannelSpectrum(const float *magnitude,
    channel_analysis_t *analysis)
{
    uint32_t relative_bin;
    signal_fft_parabolic_result_t interpolation;
    signal_harmonic_result_t harmonics;
    signal_thd_result_t thd;
    signal_snr_result_t snr;
    signal_sfdr_result_t sfdr;
    signal_harmonic_config_t harmonic_config;
    signal_snr_config_t snr_config;
    signal_sfdr_config_t sfdr_config;
    arm_max_f32(&magnitude[1], SAMPLE_COUNT / 2U,
        &analysis->peak_value, &relative_bin);
    analysis->peak_bin = relative_bin + 1U;
    if ((analysis->peak_value <= 0.0f) || (analysis->peak_bin < 2U) ||
        (analysis->peak_bin + 1U > SAMPLE_COUNT / 2U)) return false;
    if (SignalFFTParabolicInterpolation_Process(magnitude,
            SAMPLE_COUNT / 2U + 1U, analysis->peak_bin, sample_rate_hz,
            SAMPLE_COUNT, &interpolation) != SIGNAL_ALGORITHM_OK) return false;
    analysis->frequency_hz = interpolation.frequency_hz;
    harmonic_config.fundamental_frequency_hz = analysis->frequency_hz;
    harmonic_config.first_order = 1U; harmonic_config.last_order = 3U;
    harmonic_config.radius_bins = 1U;
    if (SignalHarmonic_Process(magnitude, SAMPLE_COUNT / 2U + 1U,
            sample_rate_hz, SAMPLE_COUNT, &harmonic_config,
            &harmonics) != SIGNAL_ALGORITHM_OK) return false;
    if (SignalTHD_Process(&harmonics, &thd) != SIGNAL_ALGORITHM_OK) return false;
    analysis->thd_percent = thd.thd_percent;
    if (harmonics.items[1U].root_sum_square > 0.0f) {
        analysis->harmonic_percent[0U] = 100.0f;
        analysis->harmonic_percent[1U] = 100.0f * harmonics.items[2U].root_sum_square / harmonics.items[1U].root_sum_square;
        analysis->harmonic_percent[2U] = 100.0f * harmonics.items[3U].root_sum_square / harmonics.items[1U].root_sum_square;
    }
    snr_config.signal_start_bin = analysis->peak_bin - 1U;
    snr_config.signal_end_bin = analysis->peak_bin + 1U;
    snr_config.analysis_start_bin = 1U; snr_config.analysis_end_bin = SAMPLE_COUNT / 2U;
    snr_config.excluded_ranges = NULL; snr_config.excluded_range_count = 0U;
    sfdr_config.main_start_bin = analysis->peak_bin - 1U;
    sfdr_config.main_end_bin = analysis->peak_bin + 1U;
    sfdr_config.analysis_start_bin = 1U; sfdr_config.analysis_end_bin = SAMPLE_COUNT / 2U;
    if (SignalSNR_Process(magnitude, SAMPLE_COUNT / 2U + 1U,
            &snr_config, &snr) != SIGNAL_ALGORITHM_OK) return false;
    if (SignalSFDR_Process(magnitude, SAMPLE_COUNT / 2U + 1U,
            &sfdr_config, &sfdr) != SIGNAL_ALGORITHM_OK) return false;
    analysis->snr_db = snr.snr_db; analysis->sfdr_db = sfdr.sfdr_db;
    analysis->valid = true;
    return true;
}

/* [READY_PROJECT_LOCAL] 页面与 X 轴范围只影响显示，不触发重新 FFT。 */
static void HandleKeypad(void)
{
    char key;
    while (key_queue_tail != key_queue_head) {
        key = key_queue[key_queue_tail];
        key_queue_tail = (uint8_t)((key_queue_tail + 1U) % KEY_QUEUE_SIZE);
        if (key == 'A') current_page = current_page == PAGE_CH1_SPECTRUM ?
            PAGE_SUMMARY : (app_page_t)(current_page - 1U);
        else if (key == 'B') current_page =
            (app_page_t)((current_page + 1U) % PAGE_COUNT);
        else if (key == 'C') spectrum_range_index =
            (uint8_t)((spectrum_range_index + 1U) % 4U);
        display_due = true;
    }
}

/* ============================================================
 * [函数] DrawSpectrumTrace
 * [功能] 把已有 magnitude 映射为 dB 频谱，自动选择 60 dB Y 范围。
 * [来源] [READY_PROJECT_LOCAL]；依赖 fuyong 的 magnitude 结果。
 * [输入] magnitude/analysis；[输出] TFT 曲线和 H1/H2/H3 标记。
 * [单位] x=Hz、y=dB；[全局] spectrum_range/Fs/tft。
 * [步骤] 限制显示终频→求峰 dB→每列映射 bin→画线→标谐波。
 * [原因] fuyong 22 没有完整双路 dB 自动 Y 量程。
 * [单帧唯一] 仅当前频谱页刷新时一次；[复用] CH1/CH2 参数化调用。
 * [差异] 新增 dB/自动范围；[依赖] TFT。
 * ============================================================ */
static void DrawSpectrumTrace(const float *magnitude,
    const channel_analysis_t *analysis)
{
    uint32_t x, order;
    float end_hz = spectrum_range_hz[spectrum_range_index];
    const float nyquist = 0.5f * sample_rate_hz;
    const float top_db = 20.0f * log10f(analysis->peak_value + 0.000000001f);
    const float bottom_db = top_db - 60.0f;
    if (end_hz > nyquist) end_hz = nyquist;
    (void)TFT_ST7789_DrawRect(&tft, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H, TFT_ST7789_BLUE);
    for (x = 1U; x < (uint32_t)(GRAPH_W - 2); ++x) {
        const float f0 = (float)(x - 1U) * end_hz / (float)(GRAPH_W - 3);
        const float f1 = (float)x * end_hz / (float)(GRAPH_W - 3);
        const uint32_t b0 = (uint32_t)(f0 * SAMPLE_COUNT / sample_rate_hz);
        const uint32_t b1 = (uint32_t)(f1 * SAMPLE_COUNT / sample_rate_hz);
        float d0 = 20.0f * log10f(magnitude[b0] + 0.000000001f);
        float d1 = 20.0f * log10f(magnitude[b1] + 0.000000001f);
        int32_t y0, y1;
        if (d0 < bottom_db) d0 = bottom_db; if (d0 > top_db) d0 = top_db;
        if (d1 < bottom_db) d1 = bottom_db; if (d1 > top_db) d1 = top_db;
        y0 = GRAPH_Y + GRAPH_H - 2 - (int32_t)((d0 - bottom_db) * (GRAPH_H - 3) / 60.0f);
        y1 = GRAPH_Y + GRAPH_H - 2 - (int32_t)((d1 - bottom_db) * (GRAPH_H - 3) / 60.0f);
        (void)TFT_ST7789_DrawLine(&tft, GRAPH_X + (int32_t)x, y0,
            GRAPH_X + (int32_t)x + 1, y1, TFT_ST7789_CYAN);
    }
    for (order = 1U; order <= 3U; ++order) {
        const float hz = analysis->frequency_hz * (float)order;
        if (hz <= end_hz) {
            const int32_t marker_x = GRAPH_X + 1 + (int32_t)(hz * (GRAPH_W - 3) / end_hz);
            (void)TFT_ST7789_DrawLine(&tft, marker_x, GRAPH_Y + 1,
                marker_x, GRAPH_Y + 12, order == 1U ? TFT_ST7789_YELLOW : TFT_ST7789_RED);
        }
    }
}

static void DrawChannelPage(const char *title, const float *magnitude,
    const channel_analysis_t *analysis)
{
    (void)title;
    (void)TFT_ST7789_DrawFloat(&tft, 8, 48, analysis->valid ? analysis->frequency_hz : 0.0f, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 104, 48, analysis->valid ? analysis->thd_percent : 0.0f, 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 184, 48, analysis->valid ? analysis->snr_db : 0.0f, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 256, 48, analysis->valid ? analysis->sfdr_db : 0.0f, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    if (analysis->valid) DrawSpectrumTrace(magnitude, analysis);
    (void)TFT_ST7789_DrawFloat(&tft, 88, 208, analysis->harmonic_percent[1U], 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_RED, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 168, 208, analysis->harmonic_percent[2U], 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_RED, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 248, 208, spectrum_range_hz[spectrum_range_index] / 1000.0f, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    DrawText(288, 208, "k", TFT_ST7789_WHITE);
}

/* [READY_PROJECT_LOCAL] SUMMARY 只读取两路已保存结果，不运行 FFT/分析。 */
static void DrawSummary(void)
{
    (void)TFT_ST7789_DrawFloat(&tft, 8, 60, ch1_analysis.frequency_hz, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 104, 60, ch1_analysis.thd_percent, 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 184, 60, ch1_analysis.harmonic_percent[1U], 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 256, 60, ch1_analysis.harmonic_percent[2U], 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 8, 124, ch2_analysis.frequency_hz, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 104, 124, ch2_analysis.thd_percent, 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 184, 124, ch2_analysis.harmonic_percent[1U], 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 256, 124, ch2_analysis.harmonic_percent[2U], 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 160, 176, ch1_analysis.snr_db, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 240, 176, ch1_analysis.sfdr_db, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 160, 200, ch2_analysis.snr_db, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 240, 200, ch2_analysis.sfdr_db, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
}

/* [READY_PROJECT_LOCAL] 仅首次显示或翻页时重画固定标题、标签和边框。 */
static void DrawStaticUi(void)
{
    (void)TFT_ST7789_FillScreen(&tft, TFT_ST7789_BLACK);
    if (current_page == PAGE_CH1_SPECTRUM ||
        current_page == PAGE_CH2_SPECTRUM) {
        DrawText(8, 4, current_page == PAGE_CH1_SPECTRUM ?
            "CH1 SPECTRUM" : "CH2 SPECTRUM", TFT_ST7789_CYAN);
        DrawText(8, 28, "F/THD/SNR/SFDR:", TFT_ST7789_WHITE);
        (void)TFT_ST7789_DrawRect(&tft, GRAPH_X, GRAPH_Y, GRAPH_W,
            GRAPH_H, TFT_ST7789_BLUE);
        DrawText(8, 208, "H2/H3%:", TFT_ST7789_WHITE);
        DrawText(8, 224, "A/B PAGE C RANGE", TFT_ST7789_WHITE);
    } else {
        DrawText(8, 4, "DUAL SPECTRUM SUMMARY", TFT_ST7789_CYAN);
        DrawText(8, 36, "CH1 F/THD/H2/H3", TFT_ST7789_YELLOW);
        DrawText(8, 100, "CH2 F/THD/H2/H3", TFT_ST7789_YELLOW);
        DrawText(8, 176, "SNR/SFDR CH1:", TFT_ST7789_WHITE);
        DrawText(8, 200, "SNR/SFDR CH2:", TFT_ST7789_WHITE);
        DrawText(8, 224, "A/B PAGE", TFT_ST7789_WHITE);
    }
    displayed_page = current_page;
}

static void UpdateDisplay(void)
{
    if (displayed_page != current_page) DrawStaticUi();
    if (current_page == PAGE_CH1_SPECTRUM ||
        current_page == PAGE_CH2_SPECTRUM) {
        (void)TFT_ST7789_FillRect(&tft, 0, 48, 320, 16,
            TFT_ST7789_BLACK);
        (void)TFT_ST7789_FillRect(&tft, GRAPH_X + 1, GRAPH_Y + 1,
            GRAPH_W - 2, GRAPH_H - 2, TFT_ST7789_BLACK);
        (void)TFT_ST7789_FillRect(&tft, 88, 208, 224, 16,
            TFT_ST7789_BLACK);
    } else {
        (void)TFT_ST7789_FillRect(&tft, 0, 60, 320, 16, TFT_ST7789_BLACK);
        (void)TFT_ST7789_FillRect(&tft, 0, 124, 320, 16, TFT_ST7789_BLACK);
        (void)TFT_ST7789_FillRect(&tft, 160, 176, 152, 16, TFT_ST7789_BLACK);
        (void)TFT_ST7789_FillRect(&tft, 160, 200, 152, 16, TFT_ST7789_BLACK);
    }
    if (current_page == PAGE_CH1_SPECTRUM)
        DrawChannelPage("CH1 SPECTRUM", fft_ch1_magnitude, &ch1_analysis);
    else if (current_page == PAGE_CH2_SPECTRUM)
        DrawChannelPage("CH2 SPECTRUM", fft_ch2_magnitude, &ch2_analysis);
    else DrawSummary();
    display_due = false; display_elapsed_ms = 0U;
}

/* [FUYONG_ADAPTED][moni01] 5 ms 扫描结果进入 8 项环形队列。 */
static void QueueKey(char symbol)
{
    const uint8_t next = (uint8_t)((key_queue_head + 1U) % KEY_QUEUE_SIZE);
    if (next != key_queue_tail) {
        key_queue[key_queue_head] = symbol;
        key_queue_head = next;
    }
}

void SysTick_Handler(void)
{
    static uint8_t key_ms;
    char symbol;
    ++key_ms;
    if (display_elapsed_ms < DISPLAY_PERIOD_MS) ++display_elapsed_ms; else display_due = true;
    if (key_ms < KEYPAD_SCAN_MS) return;
    key_ms = 0U;
    if (SignalMatrixKeypad4x4_ReadNewSymbol(&symbol) == SIGNAL_RESULT_OK) {
        QueueKey(symbol);
    }
}

static void App_Init(void)
{
    const signal_dual_adc_config_t config = { SAMPLE_RATE_REQUEST_HZ,
        CPUCLK_FREQ, 65536U };
    SYSCFG_DL_init();
    if (SignalDualADC_Init(&config) != SIGNAL_RESULT_OK) while (true) { }
    DL_DMA_enableInterrupt(DMA, DL_DMA_INTERRUPT_CHANNEL0 | DL_DMA_INTERRUPT_CHANNEL1);
    if (SignalTFTST7789_MSPM0_Init(&tft, TFT_ST7789_ROTATION_270, 0U, 0U) != TFT_ST7789_OK) while (true) { }
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (true) { }
}

int main(void)
{
    App_Init();
    while (true) {
        HandleKeypad();
        if (!AcquireDualADCFrame()) continue;
        PrepareSignal(adc_ch1_samples, voltage_ch1_samples, centered_ch1_samples, SAMPLE_COUNT);
        PrepareSignal(adc_ch2_samples, voltage_ch2_samples, centered_ch2_samples, SAMPLE_COUNT);
        ch1_analysis.valid = false; ch2_analysis.valid = false;
        if (RunFFTCommon(centered_ch1_samples, fft_ch1_magnitude))
            (void)AnalyzeChannelSpectrum(fft_ch1_magnitude, &ch1_analysis);
        if (RunFFTCommon(centered_ch2_samples, fft_ch2_magnitude))
            (void)AnalyzeChannelSpectrum(fft_ch2_magnitude, &ch2_analysis);
        if (display_due) UpdateDisplay();
    }
}
