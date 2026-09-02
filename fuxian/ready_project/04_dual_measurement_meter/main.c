/* ============================================================
 * 工程：04_dual_measurement_meter
 * 用途：双通道 F/Vpp/RMS/DC/Gain/Phase/Delay 综合测量仪。
 * 输入：ADC CH1=PA25，ADC CH2=PA17；输出：ST7789 TFT。
 * KEY MAP：A/B 上一/下一页；其余键保留。
 *
 * 主要来源：04_dual_adc_dma、11_zero_cross_frequency、
 * 30_basic_measurement、40_dual_channel_measurement、70、80。
 * 平台闭包来自 example01。经授权复制 modules/.syscfg；模块未修改。
 * ============================================================ */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "arm_math.h"
#include "ti_msp_dl_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_dual_adc_phase.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"

#define SAMPLE_COUNT             (512U)
#define ADC_REFERENCE_V          (3.3f)
#define REQUEST_SAMPLE_RATE_HZ   (100000U)
#define KEYPAD_SCAN_MS           (5U)
#define KEY_QUEUE_SIZE           (8U)
#define DISPLAY_PERIOD_MS        (250U)

typedef enum { PAGE_BASIC = 0U, PAGE_DUAL, PAGE_COUNT } app_page_t;
typedef struct {
    float mean_v;
    float minimum_v;
    float maximum_v;
    float vpp_v;
    float rms_v;
    float ac_rms_v;
} basic_result_t;

static uint16_t adc_ch1_samples[SAMPLE_COUNT];
static uint16_t adc_ch2_samples[SAMPLE_COUNT];
static float voltage_ch1_samples[SAMPLE_COUNT];
static float voltage_ch2_samples[SAMPLE_COUNT];
static float centered_ch1_samples[SAMPLE_COUNT];
static float centered_ch2_samples[SAMPLE_COUNT];
static float sample_rate_hz = (float)REQUEST_SAMPLE_RATE_HZ;
static float frequency_ch1_hz;
static float frequency_ch2_hz;
static float ch1_vpp_v, ch2_vpp_v;
static float ch1_rms_v, ch2_rms_v;
static float ch1_mean_v, ch2_mean_v;
static float phase_deg, delay_s, gain_ratio, gain_db;
static bool ch1_frequency_valid, ch2_frequency_valid, phase_valid;
static app_page_t current_page = PAGE_BASIC;
static app_page_t displayed_page = PAGE_COUNT;
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
 * [功能] 用同一 Timer Event 触发两路 ADC/DMA，取得同步的一帧。
 * [来源] [FUYONG_COPY] fuyong/04_dual_adc_dma。
 * [对应 COPY 区/函数] DUAL_ADC_DMA / AcquireDualADCFrame()。
 * [输入] 无；[输出] adc_ch1_samples/adc_ch2_samples。
 * [单位] ADC code；[全局] 两路原始数组。
 * [步骤] Start→等待两个 DMA finished→读取真实采样率。
 * [原因] 相位必须来自同一时刻的双路样本。
 * [单帧唯一] 是；[复用] 本帧所有 Basic/Frequency/Phase 共用。
 * [差异] 仅采用统一数组命名；[依赖] dual_adc_mspm0g3507。
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
 * [函数] ConvertADCToVoltage
 * [功能] 把任一路 ADC code 参数化换算为 V。
 * [来源] [FUYONG_ADAPTED] 30_basic_measurement/ConvertADCToVoltage。
 * [输入] input_samples、sample_count；[输出] output_samples。
 * [单位] code→V；[全局] 无。
 * [步骤] voltage=code*3.3/4095；[原因] 后续量值必须为物理单位。
 * [单帧唯一] 每通道一次；[复用] Basic 与频率共同读取。
 * [差异] 原教学版固定全局；这里只参数化，公式不变。
 * [依赖] 无。
 * ============================================================ */
static void ConvertADCToVoltage(const uint16_t *input_samples,
    float *output_samples, uint32_t sample_count)
{
    uint32_t index;
    for (index = 0U; index < sample_count; ++index) {
        output_samples[index] = (float)input_samples[index] *
            ADC_REFERENCE_V / 4095.0f;
    }
}

/* ============================================================
 * [函数] MeasureBasicParameters
 * [功能] 对任一路电压计算 DC、min/max、Vpp、RMS、AC RMS，并产生去 DC 数组。
 * [来源] [FUYONG_ADAPTED] 30_basic_measurement/MeasureBasicParameters。
 * [输入] voltage_input/sample_count；[输出] centered_output/result。
 * [单位] V；[全局] 无。
 * [步骤] CMSIS mean/min/max/rms→offset(-mean)→AC RMS。
 * [原因] 一次遍历链生成所有结果，避免 Basic 与测频重复去 DC。
 * [单帧唯一] 每通道一次；[复用] 过零频率、增益均复用结果。
 * [差异] 只做参数化；算法/CMSIS 调用保持一致。
 * [依赖] CMSIS-DSP。
 * ============================================================ */
static void MeasureBasicParameters(const float *voltage_input,
    float *centered_output, uint32_t sample_count, basic_result_t *result)
{
    uint32_t ignored_index;
    arm_mean_f32(voltage_input, sample_count, &result->mean_v);
    arm_min_f32(voltage_input, sample_count, &result->minimum_v, &ignored_index);
    arm_max_f32(voltage_input, sample_count, &result->maximum_v, &ignored_index);
    result->vpp_v = result->maximum_v - result->minimum_v;
    arm_rms_f32(voltage_input, sample_count, &result->rms_v);
    arm_offset_f32(voltage_input, -result->mean_v, centered_output, sample_count);
    arm_rms_f32(centered_output, sample_count, &result->ac_rms_v);
}

/* ============================================================
 * [函数] MeasureFrequencyZeroCross
 * [功能] 从任一路去 DC 数据的上升沿过零间隔测频，并做两点线性插值。
 * [来源] [FUYONG_ADAPTED] fuyong/11_zero_cross_frequency。
 * [输入] centered_input、count、Fs；[输出] frequency_out。
 * [单位] V、sample、Hz；[全局] 无。
 * [步骤] 找上升夹点→线性插值小数位置→平均首末间隔。
 * [原因] 多周期平均比单周期更稳，且不需要第二次采集。
 * [单帧唯一] 每通道一次；[复用] Delay 使用同一频率结果。
 * [差异] 原教学版调用模块并固定全局；此处保持线性插值公式并参数化。
 * [依赖] 无。
 * ============================================================ */
static bool MeasureFrequencyZeroCross(const float *centered_input,
    uint32_t count, float fs_hz, float *frequency_out)
{
    uint32_t index;
    uint32_t crossing_count = 0U;
    float first_position = 0.0f, last_position = 0.0f;
    for (index = 1U; index < count; ++index) {
        const float left = centered_input[index - 1U];
        const float right = centered_input[index];
        if ((left <= 0.0f) && (right > 0.0f) && (right != left)) {
            const float position = (float)(index - 1U) - left / (right - left);
            if (crossing_count == 0U) first_position = position;
            last_position = position;
            ++crossing_count;
        }
    }
    if ((crossing_count < 2U) || !(last_position > first_position)) return false;
    *frequency_out = (float)(crossing_count - 1U) * fs_hz /
        (last_position - first_position);
    return isfinite(*frequency_out) && (*frequency_out > 0.0f);
}

/* ============================================================
 * [函数] RunMeasurement
 * [功能] 对同一帧执行唯一转换/Basic/测频，并计算 Gain/Phase/Delay。
 * [来源] [FUYONG_ADAPTED] 30 + 40；Gain 为 [READY_PROJECT_LOCAL]。
 * [输入] 两路 ADC 数组；[输出] 全部测量结果。
 * [单位] V/Hz/degree/s/dB；[全局] 本文件结果变量。
 * [步骤] 两路转换→两路 Basic→两路频率→相位模块→Gain/Delay。
 * [原因] 数据链集中后不会重复 code→V 或去 DC。
 * [单帧唯一] 是；[复用] 两个 TFT 页面只读结果。
 * [差异] 相位仍调用验证模块；Gain 使用 AC RMS 比值。
 * [依赖] dual_adc_phase、CMSIS。
 * ============================================================ */
static void RunMeasurement(void)
{
    basic_result_t ch1, ch2;
    const signal_dual_adc_phase_config_t phase_config = {
        24U, 80U, 1U, SIGNAL_DUAL_ADC_PHASE_MAX_X_CROSSINGS,
        SIGNAL_DUAL_ADC_PHASE_MAX_Y_CROSSINGS
    };
    signal_dual_adc_phase_result_t result;
    ConvertADCToVoltage(adc_ch1_samples, voltage_ch1_samples, SAMPLE_COUNT);
    ConvertADCToVoltage(adc_ch2_samples, voltage_ch2_samples, SAMPLE_COUNT);
    MeasureBasicParameters(voltage_ch1_samples, centered_ch1_samples,
        SAMPLE_COUNT, &ch1);
    MeasureBasicParameters(voltage_ch2_samples, centered_ch2_samples,
        SAMPLE_COUNT, &ch2);
    ch1_mean_v = ch1.mean_v; ch2_mean_v = ch2.mean_v;
    ch1_vpp_v = ch1.vpp_v; ch2_vpp_v = ch2.vpp_v;
    ch1_rms_v = ch1.rms_v; ch2_rms_v = ch2.rms_v;
    ch1_frequency_valid = MeasureFrequencyZeroCross(centered_ch1_samples,
        SAMPLE_COUNT, sample_rate_hz, &frequency_ch1_hz);
    ch2_frequency_valid = MeasureFrequencyZeroCross(centered_ch2_samples,
        SAMPLE_COUNT, sample_rate_hz, &frequency_ch2_hz);
    phase_valid = SignalDualADCPhase_Process(adc_ch1_samples, adc_ch2_samples,
        SAMPLE_COUNT, (uint32_t)sample_rate_hz, &phase_config,
        &result) == SIGNAL_ALGORITHM_OK && result.valid != 0U;
    phase_deg = phase_valid ? (float)result.phase_degrees : 0.0f;
    gain_ratio = (ch1.ac_rms_v > 0.000001f) ? ch2.ac_rms_v / ch1.ac_rms_v : 0.0f;
    gain_db = (gain_ratio > 0.0f) ? 20.0f * log10f(gain_ratio) : -120.0f;
    if (phase_valid && ch1_frequency_valid && (frequency_ch1_hz > 0.0f))
        delay_s = phase_deg / (360.0f * frequency_ch1_hz);
    else delay_s = 0.0f;
}

/* [READY_PROJECT_LOCAL] A/B 只改变用户看到的页面，不改变测量链。 */
static void HandleKeypad(void)
{
    char key;
    while (key_queue_tail != key_queue_head) {
        key = key_queue[key_queue_tail];
        key_queue_tail = (uint8_t)((key_queue_tail + 1U) % KEY_QUEUE_SIZE);
        if ((key == 'A') || (key == 'B')) {
            current_page = current_page == PAGE_BASIC ? PAGE_DUAL : PAGE_BASIC;
            display_due = true;
        }
    }
}

/* ============================================================
 * [函数] UpdateDisplay
 * [功能] 按 current_page 显示 BASIC 或 DUAL 结果。
 * [来源] [FUYONG_ADAPTED] 80_tft_usage/DrawPage；布局为 LOCAL。
 * [输入] 测量结果；[输出] TFT；[单位] Hz/V/dB/degree/us。
 * [全局] current_page 与结果变量；[步骤] 翻页画静态标签→局部刷新数值。
 * [原因] 完整帧分析后刷新，避免 SPI 干扰采集。
 * [单帧唯一] 每 250 ms 最多一次；[复用] main。
 * [差异] 两页仪表布局；[依赖] ST7789/font。
 * ============================================================ */
static void DrawStaticUi(void)
{
    /* 整屏只在上电或翻页清一次，固定标签随后保持不动。 */
    (void)TFT_ST7789_FillScreen(&tft, TFT_ST7789_BLACK);
    if (current_page == PAGE_BASIC) {
        DrawText(8, 4, "DUAL METER - BASIC", TFT_ST7789_CYAN);
        DrawText(8, 28, "CH1 F/Vpp/RMS/DC", TFT_ST7789_YELLOW);
        DrawText(8, 108, "CH2 F/Vpp/RMS/DC", TFT_ST7789_YELLOW);
        DrawText(112, 204, "Sa/s  A/B PAGE", TFT_ST7789_WHITE);
    } else {
        DrawText(8, 4, "DUAL METER - RELATION", TFT_ST7789_CYAN);
        DrawText(8, 36, "Gain:", TFT_ST7789_WHITE);
        DrawText(8, 68, "Gain(dB):", TFT_ST7789_WHITE);
        DrawText(8, 100, "Phase(deg):", TFT_ST7789_WHITE);
        DrawText(8, 132, "Delay(us):", TFT_ST7789_WHITE);
        DrawText(8, 224, "A/B PAGE", TFT_ST7789_WHITE);
    }
    displayed_page = current_page;
}

static void UpdateDisplay(void)
{
    if (displayed_page != current_page) DrawStaticUi();
    if (current_page == PAGE_BASIC) {
        (void)TFT_ST7789_FillRect(&tft, 0, 52, 320, 40, TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawFloat(&tft, 8, 52, ch1_frequency_valid ? frequency_ch1_hz : 0.0f, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_DrawFloat(&tft, 112, 52, ch1_vpp_v, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_DrawFloat(&tft, 208, 52, ch1_rms_v, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_DrawFloat(&tft, 8, 76, ch1_mean_v, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_FillRect(&tft, 0, 132, 320, 40, TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawFloat(&tft, 8, 132, ch2_frequency_valid ? frequency_ch2_hz : 0.0f, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_DrawFloat(&tft, 112, 132, ch2_vpp_v, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_DrawFloat(&tft, 208, 132, ch2_rms_v, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_DrawFloat(&tft, 8, 156, ch2_mean_v, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_FillRect(&tft, 0, 204, 104, 16, TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawInt32(&tft, 8, 204, (int32_t)sample_rate_hz, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    } else {
        (void)TFT_ST7789_FillRect(&tft, 136, 36, 176, 16, TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawFloat(&tft, 136, 36, gain_ratio, 4U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_FillRect(&tft, 136, 68, 176, 16, TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawFloat(&tft, 136, 68, gain_db, 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_FillRect(&tft, 136, 100, 176, 16, TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawFloat(&tft, 136, 100, phase_valid ? phase_deg : 0.0f, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_FillRect(&tft, 136, 132, 176, 16, TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawFloat(&tft, 136, 132, delay_s * 1000000.0f, 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_FillRect(&tft, 0, 204, 320, 16, TFT_ST7789_BLACK);
        DrawText(8, 204, phase_valid ? "PHASE: VALID" : "PHASE: NO DATA", phase_valid ? TFT_ST7789_GREEN : TFT_ST7789_RED);
    }
    display_due = false; display_elapsed_ms = 0U;
}

/* [FUYONG_ADAPTED][moni01] 5 ms 扫描结果入队，队列满时保留旧事件。 */
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
    if (display_elapsed_ms < DISPLAY_PERIOD_MS) ++display_elapsed_ms;
    else display_due = true;
    if (key_ms < KEYPAD_SCAN_MS) return;
    key_ms = 0U;
    if (SignalMatrixKeypad4x4_ReadNewSymbol(&symbol) == SIGNAL_RESULT_OK) {
        QueueKey(symbol);
    }
}

static void App_Init(void)
{
    const signal_dual_adc_config_t config = {
        REQUEST_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
    };
    SYSCFG_DL_init();
    if (SignalDualADC_Init(&config) != SIGNAL_RESULT_OK) while (true) { }
    DL_DMA_enableInterrupt(DMA, DL_DMA_INTERRUPT_CHANNEL0 |
        DL_DMA_INTERRUPT_CHANNEL1);
    if (SignalTFTST7789_MSPM0_Init(&tft, TFT_ST7789_ROTATION_270,
            0U, 0U) != TFT_ST7789_OK) while (true) { }
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (true) { }
}

int main(void)
{
    App_Init();
    while (true) {
        HandleKeypad();
        if (!AcquireDualADCFrame()) continue;
        RunMeasurement();
        if (display_due) UpdateDisplay();
    }
}
