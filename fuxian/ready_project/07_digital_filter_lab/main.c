/* ============================================================
 * 工程：07_digital_filter_lab
 * 用途：同一 ADC frame 的 RAW/FILTERED 波形与频谱对比。
 * KEY MAP：A/B 页面；C 滤波模式；星号/井号 窗口减/增；D Hampel 阈值循环。
 * 模式：NONE、MOVING_AVERAGE、MEDIAN、HAMPEL。
 * 来源：15_filter_processing、20_fft_analysis、21_time_domain_waveform、
 * 50_robust_measurement、70、80；平台闭包来自 example04。
 * 经授权复制 modules/.syscfg，未修改模块或生成文件。
 * ============================================================ */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "arm_const_structs.h"
#include "arm_math.h"
#include "ti_msp_dl_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_hampel.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_median_filter.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_window.h"
#include "signal_window_gain_correction.h"

#define SAMPLE_COUNT            (512U)
#define SAMPLE_RATE_REQUEST_HZ  (100000U)
#define ADC_REFERENCE_V         (3.3f)
#define GRAPH_X                 (8)
#define GRAPH_Y                 (64)
#define GRAPH_W                 (304)
#define GRAPH_H                 (144)
#define KEYPAD_SCAN_MS          (5U)
#define KEY_QUEUE_SIZE          (8U)
#define DISPLAY_PERIOD_MS       (300U)

typedef enum { PAGE_WAVEFORM = 0U, PAGE_SPECTRUM, PAGE_COUNT } app_page_t;
typedef enum { FILTER_NONE = 0U, FILTER_MOVING_AVERAGE,
    FILTER_MEDIAN, FILTER_HAMPEL, FILTER_COUNT } filter_mode_t;

static uint16_t adc_samples[SAMPLE_COUNT];
static uint16_t adc_unused_samples[SAMPLE_COUNT];
static float voltage_samples[SAMPLE_COUNT];
static float filtered_samples[SAMPLE_COUNT];
static float filter_workspace[SAMPLE_COUNT];
static float fft_input[SAMPLE_COUNT];
static float raw_fft_magnitude[SAMPLE_COUNT / 2U + 1U];
static float filtered_fft_magnitude[SAMPLE_COUNT / 2U + 1U];
static q15_t fft_q15[2U * SAMPLE_COUNT];
static q15_t fft_magnitude_q15[SAMPLE_COUNT];
static float sample_rate_hz = (float)SAMPLE_RATE_REQUEST_HZ;
static app_page_t current_page = PAGE_WAVEFORM;
static app_page_t displayed_page = PAGE_COUNT;
static filter_mode_t filter_mode = FILTER_NONE;
static uint32_t filter_window_size = 5U;
static float hampel_threshold = 3.0f;
static uint32_t outlier_count;
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
 * [函数] AcquireADCFrame
 * [功能] 获取本次 RAW/FILTERED 共用的唯一 ADC 帧。
 * [来源] [FUYONG_ADAPTED] 04_dual_adc_dma/AcquireDualADCFrame。
 * [输入] CH1；[输出] adc_samples；[单位] code。
 * [全局] 原始数组/Fs；[步骤] Start→等待→读实际 Fs。
 * [原因] RAW 和 FILTERED 必须是同一信号时刻，不能采两帧比较。
 * [单帧唯一] 是；[复用] 转换、滤波、两路显示。
 * [差异] CH2 只作 DMA 占位；[依赖] dual_adc。
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
 * [函数] ConvertADCToVoltage
 * [功能] 本帧唯一一次 ADC code→V。
 * [来源] [FUYONG_ADAPTED] 15_filter_processing/FILTER_CONVERT。
 * [输入] input/count；[输出] output；[单位] code→V。
 * [全局] 无；[步骤] code*Vref/4095；[原因] 滤波和显示共用物理量。
 * [单帧唯一] 是；[复用] RAW 波形/FFT/滤波。
 * [差异] 参数化；公式不变；[依赖] 无。
 * ============================================================ */
static void ConvertADCToVoltage(const uint16_t *input, float *output,
    uint32_t count)
{
    uint32_t index;
    for (index = 0U; index < count; ++index)
        output[index] = (float)input[index] * ADC_REFERENCE_V / 4095.0f;
}

/* ============================================================
 * [函数] ApplyMovingAverage
 * [功能] 参数化滑动平均，边缘点按实际可用点数除法。
 * [来源] [FUYONG_ADAPTED] 15_filter_processing/MOVING_AVERAGE。
 * [输入] input/count/window；[输出] output；[单位] V。
 * [全局] 无；[步骤] 滑动累加并移除窗口外旧点。
 * [原因] O(N) 且比赛现场易修改；[单帧唯一] 当前模式一次。
 * [复用] ApplySelectedFilter；[差异] 窗口可调；[依赖] 无。
 * ============================================================ */
static void ApplyMovingAverage(const float *input, float *output,
    uint32_t count, uint32_t window)
{
    uint32_t index;
    float sum = 0.0f;
    for (index = 0U; index < count; ++index) {
        const uint32_t begin = index + 1U > window ? index + 1U - window : 0U;
        sum += input[index];
        if (index >= window) sum -= input[index - window];
        output[index] = sum / (float)(index - begin + 1U);
    }
}

/* ============================================================
 * [函数] ApplySelectedFilter
 * [功能] 仅运行当前选中的一种滤波器。
 * [来源] [FUYONG_ADAPTED] 15/Filter_Process + 50/Hampel。
 * [输入] voltage_samples；[输出] filtered_samples/outlier_count。
 * [单位] V/count；[全局] mode/window/threshold/workspace。
 * [步骤] NONE 复制；MA；Median 模块；Hampel 模块。
 * [原因] 不并行执行四种算法，减少 CPU；[单帧唯一] 是。
 * [复用] waveform 与 filtered FFT；[差异] 参数均为运行时可调。
 * [依赖] median_filter、hampel、CMSIS copy。
 * ============================================================ */
static bool ApplySelectedFilter(void)
{
    outlier_count = 0U;
    if (filter_mode == FILTER_NONE) {
        arm_copy_f32(voltage_samples, filtered_samples, SAMPLE_COUNT);
        return true;
    }
    if (filter_mode == FILTER_MOVING_AVERAGE) {
        ApplyMovingAverage(voltage_samples, filtered_samples, SAMPLE_COUNT,
            filter_window_size);
        return true;
    }
    if (filter_mode == FILTER_MEDIAN) {
        return SignalMedianFilter_Process(voltage_samples, filtered_samples,
            SAMPLE_COUNT, filter_window_size, filter_workspace,
            SAMPLE_COUNT) == SIGNAL_ALGORITHM_OK;
    }
    {
        const signal_hampel_config_t config = {
            filter_window_size, hampel_threshold, 0.001f
        };
        signal_hampel_result_t result;
        if (SignalHampel_Process(voltage_samples, filtered_samples,
                SAMPLE_COUNT, &config, filter_workspace, SAMPLE_COUNT,
                &result) != SIGNAL_ALGORITHM_OK) return false;
        outlier_count = result.replaced_count;
        return true;
    }
}

/* ============================================================
 * [函数] RunFFTCommon
 * [功能] 对任意一路电压加 Hann 窗并执行一次 Q15 FFT。
 * [来源] [FUYONG_ADAPTED] 20_fft_analysis/RunFFTCommon+RunQ15FFT。
 * [输入] input；[输出] magnitude_output；[单位] V/bin。
 * [全局] 共用 fft_input/fft_q15/fft_magnitude_q15。
 * [步骤] Hann→归一化 Q15→CMSIS CFFT→幅值→恢复量纲→窗增益校正。
 * [原因] 共用工作区先 RAW 后 FILTERED，避免两份 FFT workspace。
 * [单帧唯一] RAW 一次、FILTERED 一次；绘图绝不重算。
 * [复用] 频谱页面；[差异] 输入/输出参数化，算法不变。
 * [依赖] CMSIS、signal_window、window_gain_correction。
 * ============================================================ */
static bool RunFFTCommon(const float *input, float *magnitude_output)
{
    uint32_t index;
    float mean_v, max_abs = 0.0f, restore_scale;
    signal_window_result_t window_result;
    arm_mean_f32(input, SAMPLE_COUNT, &mean_v);
    arm_offset_f32(input, -mean_v, fft_input, SAMPLE_COUNT);
    if (SignalWindow_Apply(fft_input, fft_input, SAMPLE_COUNT,
            SIGNAL_WINDOW_HANN, &window_result) != SIGNAL_ALGORITHM_OK) return false;
    for (index = 0U; index < SAMPLE_COUNT; ++index) {
        const float a = fabsf(fft_input[index]);
        if (a > max_abs) max_abs = a;
    }
    if (max_abs == 0.0f) {
        arm_fill_f32(0.0f, magnitude_output, SAMPLE_COUNT / 2U + 1U);
        return true;
    }
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

/* [READY_PROJECT_LOCAL] A/B 切页，C 切模式，星号/井号保持奇数窗口，D 循环阈值。 */
static void HandleKeypad(void)
{
    char key;
    while (key_queue_tail != key_queue_head) {
        key = key_queue[key_queue_tail];
        key_queue_tail = (uint8_t)((key_queue_tail + 1U) % KEY_QUEUE_SIZE);
        if ((key == 'A') || (key == 'B'))
            current_page = current_page == PAGE_WAVEFORM ?
                PAGE_SPECTRUM : PAGE_WAVEFORM;
        else if (key == 'C') filter_mode =
            (filter_mode_t)((filter_mode + 1U) % FILTER_COUNT);
        else if ((key == '*') && (filter_window_size > 3U))
            filter_window_size -= 2U;
        else if ((key == '#') && (filter_window_size < 15U))
            filter_window_size += 2U;
        else if (key == 'D') hampel_threshold =
            hampel_threshold >= 5.0f ? 2.0f : hampel_threshold + 1.0f;
        display_due = true;
    }
}

static const char *FilterName(void)
{
    if (filter_mode == FILTER_NONE) return "NONE";
    if (filter_mode == FILTER_MOVING_AVERAGE) return "MOVING_AVERAGE";
    if (filter_mode == FILTER_MEDIAN) return "MEDIAN";
    return "HAMPEL";
}

/* [READY_PROJECT_LOCAL] 波形自动用 RAW+FILTERED 的共同 min/max 映射，便于直观比较。 */
static void DrawWaveforms(void)
{
    uint32_t x, index;
    float minimum = voltage_samples[0], maximum = voltage_samples[0];
    for (index = 0U; index < SAMPLE_COUNT; ++index) {
        if (voltage_samples[index] < minimum) minimum = voltage_samples[index];
        if (voltage_samples[index] > maximum) maximum = voltage_samples[index];
        if (filtered_samples[index] < minimum) minimum = filtered_samples[index];
        if (filtered_samples[index] > maximum) maximum = filtered_samples[index];
    }
    if (maximum - minimum < 0.01f) maximum = minimum + 0.01f;
    (void)TFT_ST7789_DrawRect(&tft, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H, TFT_ST7789_BLUE);
    for (x = 1U; x < (uint32_t)(GRAPH_W - 2); ++x) {
        const uint32_t i0 = (x - 1U) * (SAMPLE_COUNT - 1U) / (uint32_t)(GRAPH_W - 3);
        const uint32_t i1 = x * (SAMPLE_COUNT - 1U) / (uint32_t)(GRAPH_W - 3);
        const int32_t ry0 = GRAPH_Y + GRAPH_H - 2 - (int32_t)((voltage_samples[i0] - minimum) * (GRAPH_H - 3) / (maximum - minimum));
        const int32_t ry1 = GRAPH_Y + GRAPH_H - 2 - (int32_t)((voltage_samples[i1] - minimum) * (GRAPH_H - 3) / (maximum - minimum));
        const int32_t fy0 = GRAPH_Y + GRAPH_H - 2 - (int32_t)((filtered_samples[i0] - minimum) * (GRAPH_H - 3) / (maximum - minimum));
        const int32_t fy1 = GRAPH_Y + GRAPH_H - 2 - (int32_t)((filtered_samples[i1] - minimum) * (GRAPH_H - 3) / (maximum - minimum));
        (void)TFT_ST7789_DrawLine(&tft, GRAPH_X + (int32_t)x, ry0, GRAPH_X + (int32_t)x + 1, ry1, TFT_ST7789_YELLOW);
        (void)TFT_ST7789_DrawLine(&tft, GRAPH_X + (int32_t)x, fy0, GRAPH_X + (int32_t)x + 1, fy1, TFT_ST7789_CYAN);
    }
}

/* [READY_PROJECT_LOCAL] 频谱直接复用两份已算 magnitude；按共同峰值缩放。 */
static void DrawSpectra(void)
{
    uint32_t x, index;
    float peak = 0.0f;
    for (index = 1U; index <= SAMPLE_COUNT / 2U; ++index) {
        if (raw_fft_magnitude[index] > peak) peak = raw_fft_magnitude[index];
        if (filtered_fft_magnitude[index] > peak) peak = filtered_fft_magnitude[index];
    }
    if (peak <= 0.0f) peak = 1.0f;
    (void)TFT_ST7789_DrawRect(&tft, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H, TFT_ST7789_BLUE);
    for (x = 1U; x < (uint32_t)(GRAPH_W - 2); ++x) {
        const uint32_t b0 = (x - 1U) * (SAMPLE_COUNT / 2U) / (uint32_t)(GRAPH_W - 3);
        const uint32_t b1 = x * (SAMPLE_COUNT / 2U) / (uint32_t)(GRAPH_W - 3);
        const int32_t ry0 = GRAPH_Y + GRAPH_H - 2 - (int32_t)(raw_fft_magnitude[b0] * (GRAPH_H - 3) / peak);
        const int32_t ry1 = GRAPH_Y + GRAPH_H - 2 - (int32_t)(raw_fft_magnitude[b1] * (GRAPH_H - 3) / peak);
        const int32_t fy0 = GRAPH_Y + GRAPH_H - 2 - (int32_t)(filtered_fft_magnitude[b0] * (GRAPH_H - 3) / peak);
        const int32_t fy1 = GRAPH_Y + GRAPH_H - 2 - (int32_t)(filtered_fft_magnitude[b1] * (GRAPH_H - 3) / peak);
        (void)TFT_ST7789_DrawLine(&tft, GRAPH_X + (int32_t)x, ry0, GRAPH_X + (int32_t)x + 1, ry1, TFT_ST7789_YELLOW);
        (void)TFT_ST7789_DrawLine(&tft, GRAPH_X + (int32_t)x, fy0, GRAPH_X + (int32_t)x + 1, fy1, TFT_ST7789_CYAN);
    }
}

/* ============================================================
 * [函数] UpdateDisplay
 * [功能] 显示 RAW/FILTERED 波形或频谱及当前参数。
 * [来源] [FUYONG_ADAPTED] 21/22/80；映射为 LOCAL。
 * [输入] 已处理同帧数据；[输出] TFT；[单位] V/Hz/count。
 * [全局] page/mode/results；[步骤] 翻页画静态页→局部参数→局部波形/频谱。
 * [原因] 只在完整 frame 两次 FFT 后刷新一次。
 * [单帧唯一] 最多 300ms 一次；[复用] main。
 * [差异] 双 trace 共同标尺；[依赖] TFT/font。
 * ============================================================ */
static void DrawStaticUi(void)
{
    (void)TFT_ST7789_FillScreen(&tft, TFT_ST7789_BLACK);
    DrawText(8, 4, current_page == PAGE_WAVEFORM ?
        "FILTER LAB - WAVE" : "FILTER LAB - SPECTRUM", TFT_ST7789_CYAN);
    DrawText(168, 28, "W:", TFT_ST7789_WHITE);
    DrawText(232, 28, "T:", TFT_ST7789_WHITE);
    DrawText(8, 48, "RAW=YELLOW FILTERED=CYAN", TFT_ST7789_WHITE);
    (void)TFT_ST7789_DrawRect(&tft, GRAPH_X, GRAPH_Y, GRAPH_W,
        GRAPH_H, TFT_ST7789_BLUE);
    DrawText(8, 216, "A/B PAGE C MODE */# WINDOW", TFT_ST7789_WHITE);
    displayed_page = current_page;
}

static void UpdateDisplay(void)
{
    if (displayed_page != current_page) DrawStaticUi();
    (void)TFT_ST7789_FillRect(&tft, 8, 28, 152, 16, TFT_ST7789_BLACK);
    DrawText(8, 28, FilterName(), TFT_ST7789_GREEN);
    (void)TFT_ST7789_FillRect(&tft, 192, 28, 32, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawInt32(&tft, 192, 28, (int32_t)filter_window_size, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 256, 28, 64, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 256, 28, hampel_threshold, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, GRAPH_X + 1, GRAPH_Y + 1,
        GRAPH_W - 2, GRAPH_H - 2, TFT_ST7789_BLACK);
    if (current_page == PAGE_WAVEFORM) DrawWaveforms(); else DrawSpectra();
    (void)TFT_ST7789_FillRect(&tft, 280, 216, 32, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawInt32(&tft, 280, 216, (int32_t)outlier_count, TFT_ST7789_FONT_8X16, TFT_ST7789_RED, TFT_ST7789_BLACK, false);
    display_due = false; display_elapsed_ms = 0U;
}

/* [FUYONG_ADAPTED][moni01] 键盘 ISR 为单生产者，主循环为单消费者。 */
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
        ConvertADCToVoltage(adc_samples, voltage_samples, SAMPLE_COUNT);
        if (!ApplySelectedFilter()) continue;
        if (!RunFFTCommon(voltage_samples, raw_fft_magnitude)) continue;
        if (!RunFFTCommon(filtered_samples, filtered_fft_magnitude)) continue;
        if (display_due) UpdateDisplay();
    }
}
