/* ============================================================
 * 工程：01_dual_waveform_scope
 * 用途：轻量双通道示波器，支持 CH1/CH2/DUAL/XY。
 * 输入：CH1=PA25，CH2=PA17；输出：ST7789；键盘：
 * A/B 上/下一页面；C/D 减少/增加显示周期数（1~10）。
 *
 * 来源：04_dual_adc_dma、21_time_domain_waveform、21_waveform_display、
 * 24_auto_range、70、80；参考 example03。平台闭包来自 example03。
 * 经授权复制 modules/.syscfg，未修改模块或生成文件。
 * ============================================================ */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "arm_math.h"
#include "ti_msp_dl_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"

#define SAMPLE_COUNT            (512U)
#define SAMPLE_RATE_REQUEST_HZ  (100000U)
#define ADC_REFERENCE_V         (3.3f)
#define GRAPH_X                 (8)
#define GRAPH_Y                 (64)
#define GRAPH_W                 (304)
#define GRAPH_H                 (144)
#define KEYPAD_SCAN_MS          (5U)
#define KEY_QUEUE_SIZE          (8U)
#define DISPLAY_PERIOD_MS       (250U)

typedef enum { PAGE_CH1 = 0U, PAGE_CH2, PAGE_DUAL, PAGE_XY,
    PAGE_COUNT } app_page_t;

static uint16_t adc_ch1_samples[SAMPLE_COUNT];
static uint16_t adc_ch2_samples[SAMPLE_COUNT];
static float voltage_ch1_samples[SAMPLE_COUNT];
static float voltage_ch2_samples[SAMPLE_COUNT];
static float centered_ch1_samples[SAMPLE_COUNT];
static float centered_ch2_samples[SAMPLE_COUNT];
static float sample_rate_hz = (float)SAMPLE_RATE_REQUEST_HZ;
static float frequency_ch1_hz, frequency_ch2_hz;
static float ch1_half_range_v = 1.0f, ch2_half_range_v = 1.0f;
static uint8_t display_periods = 2U;
static app_page_t current_page = PAGE_DUAL;
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
 * [功能] 取得同一 Timer 触发的同步双 ADC 帧。
 * [来源] [FUYONG_COPY] 04_dual_adc_dma/DUAL_ADC_DMA。
 * [输入] 两路模拟输入；[输出] adc_ch1/adc_ch2；[单位] code。
 * [全局] 原始数组/Fs；[步骤] Start→等待双 DMA→读取实际 Fs。
 * [原因] DUAL/XY 必须逐样本对应；[单帧唯一] 是。
 * [复用] 转换、量程、频率、所有页面；[差异] 统一命名。
 * [依赖] dual_adc_mspm0g3507。
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
 * [功能] 任一路参数化 code→V。
 * [来源] [FUYONG_ADAPTED] 30/ConvertADCToVoltage。
 * [输入] input/count；[输出] output；[单位] code→V。
 * [全局] 无；[步骤] code*3.3/4095；[原因] 量程与坐标使用物理量。
 * [单帧唯一] 每通道一次；[复用] 去 DC/自动量程/绘图。
 * [差异] 原固定全局改为参数，公式不变；[依赖] 无。
 * ============================================================ */
static void ConvertADCToVoltage(const uint16_t *input, float *output,
    uint32_t count)
{
    uint32_t index;
    for (index = 0U; index < count; ++index)
        output[index] = (float)input[index] * ADC_REFERENCE_V / 4095.0f;
}

/* ============================================================
 * [函数] PrepareSignal
 * [功能] 对任一路仅一次去 DC，并估计自动纵轴半量程。
 * [来源] [FUYONG_ADAPTED] 20/PrepareSignal + 24/AUTO_RANGE。
 * [输入] voltage/count；[输出] centered/half_range；[单位] V。
 * [全局] 无；[步骤] mean→offset→max_abs×1.15→最小 20mV。
 * [原因] 独立 CH1/CH2 量程能充分利用屏幕高度。
 * [单帧唯一] 每通道一次；[复用] 所有页面。
 * [差异] 自动量程输出参数化；[依赖] CMSIS。
 * ============================================================ */
static void PrepareSignal(const float *voltage, float *centered,
    uint32_t count, float *half_range_v)
{
    uint32_t index;
    float mean_v, maximum = 0.0f;
    arm_mean_f32(voltage, count, &mean_v);
    arm_offset_f32(voltage, -mean_v, centered, count);
    for (index = 0U; index < count; ++index) {
        const float value = fabsf(centered[index]);
        if (value > maximum) maximum = value;
    }
    *half_range_v = maximum * 1.15f;
    if (*half_range_v < 0.02f) *half_range_v = 0.02f;
}

/* [FUYONG_ADAPTED] 11_zero_cross_frequency：只为“显示周期数”选择窗口，
 * 不作为精密测量结果。用多上升沿平均，失败时返回 0 并显示整帧。 */
static float MeasureFrequencyZeroCross(const float *samples, uint32_t count)
{
    uint32_t index, crossings = 0U;
    float first = 0.0f, last = 0.0f;
    for (index = 1U; index < count; ++index) {
        const float a = samples[index - 1U], b = samples[index];
        if ((a <= 0.0f) && (b > 0.0f) && (b != a)) {
            const float position = (float)(index - 1U) - a / (b - a);
            if (crossings == 0U) first = position;
            last = position; ++crossings;
        }
    }
    return (crossings >= 2U && last > first) ?
        (float)(crossings - 1U) * sample_rate_hz / (last - first) : 0.0f;
}

/* [READY_PROJECT_LOCAL]
 * 根据 CH1（无效时 CH2）频率和 display_periods 计算可见点数。
 * 最少 16 点、最多 SAMPLE_COUNT；随后按屏宽重采样，不复制显示缓冲。 */
static uint32_t VisibleSampleCount(void)
{
    const float reference_hz = frequency_ch1_hz > 0.0f ?
        frequency_ch1_hz : frequency_ch2_hz;
    uint32_t count;
    if (reference_hz <= 0.0f) return SAMPLE_COUNT;
    count = (uint32_t)((float)display_periods * sample_rate_hz / reference_hz + 0.5f);
    if (count < 16U) count = 16U;
    if (count > SAMPLE_COUNT) count = SAMPLE_COUNT;
    return count;
}

static int32_t MapY(float value, float half_range)
{
    int32_t y = GRAPH_Y + GRAPH_H / 2 -
        (int32_t)(value * (float)(GRAPH_H / 2 - 2) / half_range);
    if (y < GRAPH_Y + 1) y = GRAPH_Y + 1;
    if (y > GRAPH_Y + GRAPH_H - 2) y = GRAPH_Y + GRAPH_H - 2;
    return y;
}

/* ============================================================
 * [函数] DrawTimeDomainWaveform
 * [功能] 参数化绘制任一路时域折线，并按可见点数抽点。
 * [来源] [FUYONG_ADAPTED] 21/DrawTimeDomainWaveform。
 * [输入] samples/range/visible/color；[输出] TFT 折线。
 * [单位] V/pixel；[全局] tft。
 * [步骤] 每屏列映射两个源索引→MapY→DrawLine。
 * [原因] 不申请额外显示 buffer，屏宽固定 304。
 * [单帧唯一] 当前页面每个可见通道一次；[复用] CH1/CH2/DUAL。
 * [差异] 输入/量程/窗口参数化；[依赖] TFT。
 * ============================================================ */
static void DrawTimeDomainWaveform(const float *samples, float half_range,
    uint32_t visible_count, uint16_t color)
{
    uint32_t x;
    for (x = 1U; x < (uint32_t)(GRAPH_W - 2); ++x) {
        const uint32_t i0 = (x - 1U) * (visible_count - 1U) / (uint32_t)(GRAPH_W - 3);
        const uint32_t i1 = x * (visible_count - 1U) / (uint32_t)(GRAPH_W - 3);
        (void)TFT_ST7789_DrawLine(&tft, GRAPH_X + (int32_t)x,
            MapY(samples[i0], half_range), GRAPH_X + (int32_t)x + 1,
            MapY(samples[i1], half_range), color);
    }
}

/* [READY_PROJECT_LOCAL]
 * XY/Lissajous：CH1 决定 X、CH2 决定 Y，各自使用独立自动量程。
 * 同下标来自同步 ADC，故点对有真实相位含义。 */
static void DrawXY(void)
{
    uint32_t point;
    for (point = 1U; point < SAMPLE_COUNT; ++point) {
        int32_t x0 = GRAPH_X + GRAPH_W / 2 + (int32_t)(centered_ch1_samples[point - 1U] * (GRAPH_W / 2 - 2) / ch1_half_range_v);
        int32_t x1 = GRAPH_X + GRAPH_W / 2 + (int32_t)(centered_ch1_samples[point] * (GRAPH_W / 2 - 2) / ch1_half_range_v);
        int32_t y0 = MapY(centered_ch2_samples[point - 1U], ch2_half_range_v);
        int32_t y1 = MapY(centered_ch2_samples[point], ch2_half_range_v);
        if (x0 < GRAPH_X + 1) x0 = GRAPH_X + 1; if (x0 > GRAPH_X + GRAPH_W - 2) x0 = GRAPH_X + GRAPH_W - 2;
        if (x1 < GRAPH_X + 1) x1 = GRAPH_X + 1; if (x1 > GRAPH_X + GRAPH_W - 2) x1 = GRAPH_X + GRAPH_W - 2;
        (void)TFT_ST7789_DrawLine(&tft, x0, y0, x1, y1, TFT_ST7789_MAGENTA);
    }
}

/* [READY_PROJECT_LOCAL] A/B 只切页面；C/D 限幅周期数 1~10。 */
static void HandleKeypad(void)
{
    char key;
    /*
     * [接口适配：moni01]
     * SysTick 只生产按键事件，本函数在主循环逐个消费环形队列。
     * 即使 TFT/ADC 一帧处理时间较长，连续按下的多个键也不会被单变量覆盖。
     */
    while (key_queue_tail != key_queue_head) {
        key = key_queue[key_queue_tail];
        key_queue_tail = (uint8_t)((key_queue_tail + 1U) % KEY_QUEUE_SIZE);
        if (key == 'A') current_page = current_page == PAGE_CH1 ?
            PAGE_XY : (app_page_t)(current_page - 1U);
        else if (key == 'B') current_page =
            (app_page_t)((current_page + 1U) % PAGE_COUNT);
        else if ((key == 'C') && (display_periods > 1U)) --display_periods;
        else if ((key == 'D') && (display_periods < 10U)) ++display_periods;
        display_due = true;
    }
}

static const char *PageName(void)
{
    if (current_page == PAGE_CH1) return "CH1";
    if (current_page == PAGE_CH2) return "CH2";
    if (current_page == PAGE_DUAL) return "DUAL";
    return "XY";
}

/* ============================================================
 * [函数] UpdateDisplay
 * [功能] 显示模式/Fs/时间跨度/双路 Vdiv，并绘制对应波形。
 * [来源] [FUYONG_ADAPTED] 21/80；页面和 XY 为 LOCAL。
 * [输入] 本帧 prepared 数据；[输出] TFT；[单位] Sa/s/ms/Vdiv。
 * [全局] page/range/frequency；[步骤] 必要时画静态页→局部清数值/波形→trace。
 * [原因] 完整 DMA frame 后刷新，避免 SPI 影响采集。
 * [单帧唯一] 最多 250ms 一次；[复用] main。
 * [差异] 四页面和独立自动量程；[依赖] TFT/font。
 * ============================================================ */
static void DrawStaticUi(void)
{
    /*
     * [READY_PROJECT_LOCAL]
     * 整屏清除仅允许发生在上电后的第一次显示或页面切换时。
     * 固定标题、标签、边框和按键提示画好后，普通帧不再重复传输这些像素。
     */
    (void)TFT_ST7789_FillScreen(&tft, TFT_ST7789_BLACK);
    DrawText(8, 4, "DUAL WAVEFORM SCOPE", TFT_ST7789_CYAN);
    DrawText(224, 4, PageName(), TFT_ST7789_YELLOW);
    DrawText(8, 24, "Fs/Span/Periods:", TFT_ST7789_WHITE);
    (void)TFT_ST7789_DrawRect(&tft, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H,
        TFT_ST7789_BLUE);
    DrawText(8, 212, "CH1/CH2 Vdiv:", TFT_ST7789_WHITE);
    DrawText(8, 228, "A/B PAGE C/D PERIODS", TFT_ST7789_WHITE);
    displayed_page = current_page;
}

static void UpdateDisplay(void)
{
    const uint32_t visible = VisibleSampleCount();
    const float span_ms = (float)visible * 1000.0f / sample_rate_hz;
    if (displayed_page != current_page) DrawStaticUi();

    /* 数字行可能由 100000 变为 9999，先只清除该行，防止旧字符残留。 */
    (void)TFT_ST7789_FillRect(&tft, 0, 44, 320, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawInt32(&tft, 8, 44, (int32_t)sample_rate_hz, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 128, 44, span_ms, 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawInt32(&tft, 248, 44, display_periods, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);

    /* 仅清波形框内部；保留外框和屏幕其余文字，再补画被清掉的中心轴。 */
    (void)TFT_ST7789_FillRect(&tft, GRAPH_X + 1, GRAPH_Y + 1,
        GRAPH_W - 2, GRAPH_H - 2, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawLine(&tft, GRAPH_X + 1, GRAPH_Y + GRAPH_H / 2,
        GRAPH_X + GRAPH_W - 2, GRAPH_Y + GRAPH_H / 2, TFT_ST7789_BLUE);
    (void)TFT_ST7789_DrawLine(&tft, GRAPH_X + GRAPH_W / 2, GRAPH_Y + 1,
        GRAPH_X + GRAPH_W / 2, GRAPH_Y + GRAPH_H - 2, TFT_ST7789_BLUE);
    if ((current_page == PAGE_CH1) || (current_page == PAGE_DUAL))
        DrawTimeDomainWaveform(centered_ch1_samples, ch1_half_range_v, visible, TFT_ST7789_YELLOW);
    if ((current_page == PAGE_CH2) || (current_page == PAGE_DUAL))
        DrawTimeDomainWaveform(centered_ch2_samples, ch2_half_range_v, visible, TFT_ST7789_CYAN);
    if (current_page == PAGE_XY) DrawXY();
    (void)TFT_ST7789_FillRect(&tft, 136, 212, 176, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 136, 212, ch1_half_range_v / 4.0f, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 224, 212, ch2_half_range_v / 4.0f, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
    display_due = false; display_elapsed_ms = 0U;
}

/* [FUYONG_ADAPTED][moni01] ISR 单生产者入队；队列满时保留旧事件。 */
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
        ConvertADCToVoltage(adc_ch1_samples, voltage_ch1_samples, SAMPLE_COUNT);
        ConvertADCToVoltage(adc_ch2_samples, voltage_ch2_samples, SAMPLE_COUNT);
        PrepareSignal(voltage_ch1_samples, centered_ch1_samples, SAMPLE_COUNT, &ch1_half_range_v);
        PrepareSignal(voltage_ch2_samples, centered_ch2_samples, SAMPLE_COUNT, &ch2_half_range_v);
        frequency_ch1_hz = MeasureFrequencyZeroCross(centered_ch1_samples, SAMPLE_COUNT);
        frequency_ch2_hz = MeasureFrequencyZeroCross(centered_ch2_samples, SAMPLE_COUNT);
        if (display_due) UpdateDisplay();
    }
}
