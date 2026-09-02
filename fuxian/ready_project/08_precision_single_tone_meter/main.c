/* ============================================================
 * 工程：08_precision_single_tone_meter
 * 用途：接近正弦的单频信号 FAST/NORMAL/PRECISION 测量。
 * 输入：ADC CH1=PA25；输出：ST7789；键盘 A/B 切精度模式。
 *
 * 最终频率选择：FAST=整数 FFT；NORMAL=FFT 三点插值；
 * PRECISION=以插值 FFT 为初值的 Sine Fit 4P。三个原始结果分别保留，
 * 绝不让同一变量覆盖不同算法的物理含义。
 *
 * 来源：11、20、30、60、70、80；平台闭包来自 example04。
 * 经授权复制 modules/.syscfg；未修改任何模块或生成文件。
 * ============================================================ */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "arm_const_structs.h"
#include "arm_math.h"
#include "ti_msp_dl_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_fft_parabolic_interpolation.h"
#include "signal_harmonic.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_sine_fit_4param.h"
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
#define KEYPAD_SCAN_MS          (5U)
#define KEY_QUEUE_SIZE          (8U)
#define DISPLAY_PERIOD_MS       (300U)

typedef enum { MODE_FAST = 0U, MODE_NORMAL, MODE_PRECISION,
    MODE_COUNT } precision_mode_t;
typedef enum { PAGE_MEASUREMENT = 0U } app_page_t;

static uint16_t adc_samples[SAMPLE_COUNT];
static uint16_t adc_unused_samples[SAMPLE_COUNT];
static float voltage_samples[SAMPLE_COUNT];
static float centered_samples[SAMPLE_COUNT];
static float fft_input[SAMPLE_COUNT];
static float fft_magnitude[SAMPLE_COUNT / 2U + 1U];
static q15_t fft_q15[2U * SAMPLE_COUNT];
static q15_t fft_magnitude_q15[SAMPLE_COUNT];
static float sample_rate_hz = (float)SAMPLE_RATE_REQUEST_HZ;
static float mean_v, vpp_v, rms_v, ac_rms_v;
static float amplitude_v;
static float fft_frequency_hz;
static float zero_cross_frequency_hz;
static float sine_fit_frequency_hz;
static float frequency_hz;
static float thd_percent;
static float snr_db;
static app_page_t current_page = PAGE_MEASUREMENT;
static uint32_t peak_bin;
static float peak_value;
static bool fft_valid, zero_cross_valid, sine_fit_valid, quality_valid;
static precision_mode_t measurement_mode = MODE_NORMAL;
static tft_st7789_t tft;
static volatile char key_queue[KEY_QUEUE_SIZE];
static volatile uint8_t key_queue_head;
static volatile uint8_t key_queue_tail;
static volatile uint16_t display_elapsed_ms;
static volatile bool display_due = true;
static bool static_ui_drawn;

static void DrawText(int32_t x, int32_t y, const char *text, uint16_t color)
{
    (void)TFT_ST7789_DrawString(&tft, x, y, text, TFT_ST7789_FONT_8X16,
        color, TFT_ST7789_BLACK, false, false);
}

/* ============================================================
 * [函数] AcquireADCFrame
 * [功能] 获取本次所有算法共同使用的唯一 ADC 帧。
 * [来源] [FUYONG_ADAPTED] 04_dual_adc_dma。
 * [输入] CH1；[输出] adc_samples；[单位] code。
 * [全局] 原始数组/Fs；[步骤] Start→等待→实际 Fs。
 * [原因] FFT/ZeroCross/SineFit 必须比较同一帧。
 * [单帧唯一] 是；[复用] 全测量链；[差异] CH2 占位。
 * [依赖] dual_adc。
 * ============================================================ */
static bool AcquireADCFrame(void)
{
    if (SignalDualADC_Start(adc_samples, adc_unused_samples,
            SAMPLE_COUNT) != SIGNAL_RESULT_OK) return false;
    while (!SignalDualADC_IsFinished()) __WFI();
    sample_rate_hz = (float)SignalDualADC_GetConfiguredRate();
    return sample_rate_hz > 0.0f;
}

/* ============================================================
 * [函数] PrepareSignalAndBasic
 * [功能] 唯一一次 code→V、Basic 统计和去 DC。
 * [来源] [FUYONG_ADAPTED] 30/ConvertADCToVoltage+MeasureBasicParameters。
 * [输入] adc_samples；[输出] voltage/centered 与 DC/Vpp/RMS/AC RMS。
 * [单位] code→V；[全局] 数据与 Basic 结果。
 * [步骤] 换算→CMSIS mean/min/max/rms→offset→AC RMS。
 * [原因] 后续三个测频算法复用，不重复转换或去 DC。
 * [单帧唯一] 是；[复用] FFT/过零/Fit；[差异] 合并数据节点。
 * [依赖] CMSIS-DSP。
 * ============================================================ */
static void PrepareSignalAndBasic(void)
{
    uint32_t index, ignored;
    float minimum_v, maximum_v;
    for (index = 0U; index < SAMPLE_COUNT; ++index)
        voltage_samples[index] = (float)adc_samples[index] * ADC_REFERENCE_V / 4095.0f;
    arm_mean_f32(voltage_samples, SAMPLE_COUNT, &mean_v);
    arm_min_f32(voltage_samples, SAMPLE_COUNT, &minimum_v, &ignored);
    arm_max_f32(voltage_samples, SAMPLE_COUNT, &maximum_v, &ignored);
    vpp_v = maximum_v - minimum_v;
    amplitude_v = 0.5f * vpp_v;
    arm_rms_f32(voltage_samples, SAMPLE_COUNT, &rms_v);
    arm_offset_f32(voltage_samples, -mean_v, centered_samples, SAMPLE_COUNT);
    arm_rms_f32(centered_samples, SAMPLE_COUNT, &ac_rms_v);
}

/* ============================================================
 * [函数] RunFFTCommon
 * [功能] Hann+Q15 FFT 每帧唯一一次，产生公共 fft_magnitude。
 * [来源] [FUYONG_ADAPTED] 20/RunQ15FFT+RunFFTCommon。
 * [输入] centered_samples；[输出] fft_magnitude/peak_bin/fft_frequency。
 * [单位] V/bin/Hz；[全局] FFT 工作区。
 * [步骤] Hann→归一化→CFFT→幅值→量纲/窗增益恢复→找峰。
 * [原因] THD/SNR/插值全部复用，禁止重新 FFT。
 * [单帧唯一] 是；[复用] Normal/Precision/质量指标/显示。
 * [差异] 固定验证过的 512 点低 RAM链；[依赖] CMSIS/window。
 * ============================================================ */
static bool RunFFTCommon(void)
{
    uint32_t index, relative_bin;
    float max_abs = 0.0f, restore_scale;
    signal_window_result_t window_result;
    if (SignalWindow_Apply(centered_samples, fft_input, SAMPLE_COUNT,
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
        fft_magnitude[index] = (float)fft_magnitude_q15[index] * restore_scale;
    if (SignalWindowGainCorrection_Apply(fft_magnitude, fft_magnitude,
            SAMPLE_COUNT / 2U + 1U, SAMPLE_COUNT,
            window_result.coherent_gain) != SIGNAL_ALGORITHM_OK) return false;
    arm_max_f32(&fft_magnitude[1], SAMPLE_COUNT / 2U, &peak_value, &relative_bin);
    peak_bin = relative_bin + 1U;
    fft_frequency_hz = (float)peak_bin * sample_rate_hz / (float)SAMPLE_COUNT;
    return peak_value > 0.0f;
}

/* ============================================================
 * [函数] MeasureFrequencyZeroCross
 * [功能] 多上升沿线性插值测频。
 * [来源] [FUYONG_ADAPTED] 11/PrepareSignal+MeasureFrequencyZeroCross。
 * [输入] centered_samples/Fs；[输出] zero_cross_frequency_hz。
 * [单位] V/sample/Hz；[全局] centered 与结果。
 * [步骤] 找夹点→小数位置→首末多周期平均。
 * [原因] 提供独立于频域的交叉检查；[单帧唯一] NORMAL/PRECISION 一次。
 * [复用] 页面与诊断；[差异] 参数固定为上升沿；[依赖] 无。
 * ============================================================ */
static bool MeasureFrequencyZeroCross(void)
{
    uint32_t index, count = 0U;
    float first = 0.0f, last = 0.0f;
    for (index = 1U; index < SAMPLE_COUNT; ++index) {
        const float a = centered_samples[index - 1U];
        const float b = centered_samples[index];
        if ((a <= 0.0f) && (b > 0.0f) && (b != a)) {
            const float position = (float)(index - 1U) - a / (b - a);
            if (count == 0U) first = position;
            last = position; ++count;
        }
    }
    if ((count < 2U) || !(last > first)) return false;
    zero_cross_frequency_hz = (float)(count - 1U) * sample_rate_hz / (last - first);
    return isfinite(zero_cross_frequency_hz) && zero_cross_frequency_hz > 0.0f;
}

/* ============================================================
 * [函数] RefineFFTFrequency
 * [功能] 用相邻三谱线插值得到小数 bin 频率。
 * [来源] [FUYONG_COPY] 20/RefineFFTFrequency。
 * [输入] fft_magnitude/peak_bin/Fs；[输出] fft_frequency_hz。
 * [单位] Hz；[全局] FFT 结果。
 * [步骤] 调用 parabolic module；[原因] 降低非整周期栅栏误差。
 * [单帧唯一] NORMAL/PRECISION 一次；[复用] Fit 初值。
 * [差异] 输出写入专属变量；[依赖] fft_parabolic_interpolation。
 * ============================================================ */
static bool RefineFFTFrequency(void)
{
    signal_fft_parabolic_result_t result;
    if (SignalFFTParabolicInterpolation_Process(fft_magnitude,
            SAMPLE_COUNT / 2U + 1U, peak_bin, sample_rate_hz,
            SAMPLE_COUNT, &result) != SIGNAL_ALGORITHM_OK) return false;
    fft_frequency_hz = result.frequency_hz;
    return true;
}

/* ============================================================
 * [函数] AnalyzeQuality
 * [功能] 从同一 FFT 计算 H1~H5/THD 与 SNR。
 * [来源] [FUYONG_ADAPTED] 20/AnalyzeHarmonicsAndTHD+AnalyzeSNRAndSFDR。
 * [输入] magnitude/fft_frequency/peak_bin；[输出] thd_percent/snr_db。
 * [单位] %/dB；[全局] 质量结果。
 * [步骤] Harmonic→THD；主峰 band 与其余 bin→SNR。
 * [原因] 质量指标复用一次 FFT；[单帧唯一] NORMAL/PRECISION 一次。
 * [复用] 页面；[差异] 仅保留所需 THD/SNR；[依赖] harmonic/thd/snr。
 * ============================================================ */
static bool AnalyzeQuality(void)
{
    signal_harmonic_result_t harmonics;
    signal_thd_result_t thd;
    signal_snr_result_t snr;
    const signal_harmonic_config_t harmonic_config = {
        fft_frequency_hz, 1U, 5U, 1U
    };
    signal_snr_config_t snr_config;
    if ((peak_bin < 2U) || (peak_bin + 1U > SAMPLE_COUNT / 2U)) return false;
    if (SignalHarmonic_Process(fft_magnitude, SAMPLE_COUNT / 2U + 1U,
            sample_rate_hz, SAMPLE_COUNT, &harmonic_config,
            &harmonics) != SIGNAL_ALGORITHM_OK) return false;
    if (SignalTHD_Process(&harmonics, &thd) != SIGNAL_ALGORITHM_OK) return false;
    snr_config.signal_start_bin = peak_bin - 1U;
    snr_config.signal_end_bin = peak_bin + 1U;
    snr_config.analysis_start_bin = 1U;
    snr_config.analysis_end_bin = SAMPLE_COUNT / 2U;
    snr_config.excluded_ranges = NULL;
    snr_config.excluded_range_count = 0U;
    if (SignalSNR_Process(fft_magnitude, SAMPLE_COUNT / 2U + 1U,
            &snr_config, &snr) != SIGNAL_ALGORITHM_OK) return false;
    thd_percent = thd.thd_percent; snr_db = snr.snr_db;
    return true;
}

/* ============================================================
 * [函数] RunSineFit4Param
 * [功能] 以本帧插值 FFT 为初值做窄带 4P 正弦拟合。
 * [来源] [FUYONG_ADAPTED] 60/RunSineFit4Param。
 * [输入] voltage_samples/fft_frequency/Fs；[输出] sine_fit_frequency、amplitude。
 * [单位] V/Hz；[全局] 专属 Fit 结果。
 * [步骤] 设置半带宽为两 FFT bin→12 次搜索→保存频率/峰值幅度。
 * [原因] 初值来自同一帧，且不覆盖 FFT/ZeroCross 结果。
 * [单帧唯一] PRECISION 一次；[复用] 最终推荐值与页面。
 * [差异] 搜索宽度随 Fs/N；算法调用不变；[依赖] sine_fit_4param。
 * ============================================================ */
static bool RunSineFit4Param(void)
{
    signal_sine_fit_4param_result_t result;
    const signal_sine_fit_4param_config_t config = {
        fft_frequency_hz, 2.0f * sample_rate_hz / (float)SAMPLE_COUNT,
        sample_rate_hz, 12U
    };
    if (SignalSineFit4Param_Process(voltage_samples, SAMPLE_COUNT,
            &config, &result) != SIGNAL_ALGORITHM_OK) return false;
    sine_fit_frequency_hz = result.frequency_hz;
    amplitude_v = result.waveform.amplitude_peak_v;
    return true;
}

/* ============================================================
 * [函数] RunMeasurement
 * [功能] 按模式调度算法并明确选择最终 frequency_hz。
 * [来源] [READY_PROJECT_LOCAL]，内部步骤全部复用上述 fuyong 函数。
 * [输入] 同一 ADC 帧；[输出] 三种频率与最终推荐值。
 * [单位] Hz；[全局] mode/valid/results。
 * [步骤] Basic→一次 FFT；NORMAL 加插值/过零/质量；PRECISION 再 Fit。
 * [原因] FAST 控制运算量，PRECISION 只在需要时运行迭代拟合。
 * [单帧唯一] 是；[复用] 显示只读结果。
 * [差异] 明确优先级 Fit>插值 FFT>整数 FFT。
 * [依赖] 上述函数。
 * ============================================================ */
static void RunMeasurement(void)
{
    PrepareSignalAndBasic();
    fft_valid = RunFFTCommon();
    zero_cross_valid = false; sine_fit_valid = false; quality_valid = false;
    if (!fft_valid) { frequency_hz = 0.0f; return; }
    frequency_hz = fft_frequency_hz;
    if (measurement_mode != MODE_FAST) {
        (void)RefineFFTFrequency();
        zero_cross_valid = MeasureFrequencyZeroCross();
        quality_valid = AnalyzeQuality();
        frequency_hz = fft_frequency_hz;
    }
    if (measurement_mode == MODE_PRECISION) {
        sine_fit_valid = RunSineFit4Param();
        if (sine_fit_valid) frequency_hz = sine_fit_frequency_hz;
    }
}

static const char *ModeName(void)
{
    if (measurement_mode == MODE_FAST) return "FAST";
    if (measurement_mode == MODE_NORMAL) return "NORMAL";
    return "PRECISION";
}

/* [READY_PROJECT_LOCAL] A/B 循环精度模式；模式改变只影响下一帧调度。 */
static void HandleKeypad(void)
{
    char key;
    while (key_queue_tail != key_queue_head) {
        key = key_queue[key_queue_tail];
        key_queue_tail = (uint8_t)((key_queue_tail + 1U) % KEY_QUEUE_SIZE);
        if (key == 'A') measurement_mode = measurement_mode == MODE_FAST ?
            MODE_PRECISION : (precision_mode_t)(measurement_mode - 1U);
        else if (key == 'B') measurement_mode =
            (precision_mode_t)((measurement_mode + 1U) % MODE_COUNT);
        display_due = true;
    }
}

/* ============================================================
 * [函数] UpdateDisplay
 * [功能] 同屏显示最终值、Basic、THD/SNR 和三种测频结果。
 * [来源] [FUYONG_ADAPTED] 80/DrawPage；布局为 LOCAL。
 * [输入] 本帧结果；[输出] TFT；[单位] Hz/V/%/dB。
 * [全局] mode/results；[步骤] 首次画静态标签→仅清除并更新各数值区域。
 * [原因] 不隐藏算法差异，现场可直接比较异常结果。
 * [单帧唯一] 最多 300ms 一次；[复用] main。
 * [差异] 三频率变量分离；[依赖] ST7789/font。
 * ============================================================ */
static void UpdateDisplay(void)
{
    /*
     * [READY_PROJECT_LOCAL]
     * 精度模式由 precision_mode_t 管理，页面由 app_page_t 管理；
     * 两种状态不共用变量，避免“显示页面”和“测量算法”物理含义混淆。
     */
    if (current_page != PAGE_MEASUREMENT) {
        return;
    }
    if (!static_ui_drawn) {
        (void)TFT_ST7789_FillScreen(&tft, TFT_ST7789_BLACK);
        DrawText(8, 4, "PRECISION TONE METER", TFT_ST7789_CYAN);
        DrawText(8, 28, "F recommend:", TFT_ST7789_WHITE);
        DrawText(8, 52, "Amp/Vpp/RMS/DC:", TFT_ST7789_WHITE);
        DrawText(8, 100, "THD/SNR:", TFT_ST7789_WHITE);
        DrawText(8, 132, "FFT:", TFT_ST7789_WHITE);
        DrawText(8, 156, "ZeroCross:", TFT_ST7789_WHITE);
        DrawText(8, 180, "SineFit:", TFT_ST7789_WHITE);
        DrawText(8, 216, "A/B MODE  AC_RMS:", TFT_ST7789_WHITE);
        static_ui_drawn = true;
    }
    (void)TFT_ST7789_FillRect(&tft, 232, 4, 88, 16, TFT_ST7789_BLACK);
    DrawText(232, 4, ModeName(), TFT_ST7789_YELLOW);
    (void)TFT_ST7789_FillRect(&tft, 136, 28, 176, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 136, 28, frequency_hz, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 0, 76, 320, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 8, 76, amplitude_v, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 88, 76, vpp_v, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 168, 76, rms_v, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 248, 76, mean_v, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 104, 100, 208, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 104, 100, quality_valid ? thd_percent : 0.0f, 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 216, 100, quality_valid ? snr_db : 0.0f, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 88, 132, 224, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 88, 132, fft_valid ? fft_frequency_hz : 0.0f, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 88, 156, 224, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 88, 156, zero_cross_valid ? zero_cross_frequency_hz : 0.0f, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 88, 180, 224, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 88, 180, sine_fit_valid ? sine_fit_frequency_hz : 0.0f, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 200, 216, 112, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 200, 216, ac_rms_v, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    display_due = false; display_elapsed_ms = 0U;
}

/* [FUYONG_ADAPTED][moni01] 键盘事件由 SysTick 入队，主循环逐个消费。 */
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
        if (!AcquireADCFrame()) continue;
        RunMeasurement();
        if (display_due) UpdateDisplay();
    }
}
