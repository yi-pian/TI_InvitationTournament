/* example01 精简闭环：双 ADC 采样、ST7789 波形显示和双路相位差测量。
 * 模块 API 按 README 复制；App_* 只负责坐标映射、键盘队列、页面和显示逻辑。
 * 默认显示范围、颜色和位置在 APP_* 宏中改，矩阵键盘 GPIO 在模块固定配置中改。 */
/* fuyong REUSE: 04_dual_adc_dma/DUAL_ADC_DMA +
 * 40_dual_channel_measurement/PHASE + 70_keypad_usage + 80_tft_usage.
 * 保留本题双 trace、按键队列和相位页面状态机。 */
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_dual_adc_phase.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"

#include "signal_zero_cross.h"
#include "signal_zero_cross_interpolation.h"
#include "arm_math.h"

#include "arm_const_structs.h"
#include "signal_window.h"
#include "signal_window_gain_correction.h"
#include "signal_fft_parabolic_interpolation.h"
#include "signal_harmonic.h"
#include "signal_thd.h"

#define APP_PLOT_X       (8)
#define APP_PLOT_Y       (40)
#define APP_PLOT_W       (220)
#define APP_PLOT_H       (184)
#define APP_ADC_MID      (2048U)
#define APP_TRACE_POINT_COUNT      (220U)
#define APP_KEYPAD_SCAN_PERIOD_MS  (5U)
#define APP_KEY_QUEUE_SIZE         (8U)
#define APP_STATUS_X     (232)
#define APP_STATUS_Y     (40)
#define APP_STATUS_W     (84)
#define APP_STATUS_H     (184)
#define APP_PHASE_HYSTERESIS_CODE (16U)
#define APP_PHASE_MIN_AMPLITUDE   (64U)

#define ZERO_EVENT_CAPACITY (128U)
#define APP_PAGE_COUNT (4U)
/* g_raw_x/g_raw_y：X、Y 两路 ADC 原始码；g_sample_rate：当前采样率；g_threshold：
 * 过零门限；g_phase_*：相位测量结果和有效标志；g_key_queue/head/tail：按键环形队列；
 * revision 变量用于数值变化时局部刷新。 */
static uint16_t g_raw_x[SIGNAL_SAMPLE_COUNT];
static uint16_t g_raw_y[SIGNAL_SAMPLE_COUNT];
static tft_st7789_t g_tft;
static uint32_t g_sample_rate = SIGNAL_SAMPLE_RATE_HZ;
static uint16_t g_threshold = APP_ADC_MID;
static int32_t g_phase_degrees;
static uint8_t g_phase_valid;
static uint8_t g_phase_display_divider;
static signal_dual_adc_phase_config_t g_phase_config = {
    APP_PHASE_HYSTERESIS_CODE,
    APP_PHASE_MIN_AMPLITUDE,
    1U,
    16U,
    64U
};
static volatile signal_result_t g_adc_status;
static volatile signal_result_t g_keypad_status;
static volatile char g_key_queue[APP_KEY_QUEUE_SIZE];
static volatile uint8_t g_key_queue_head;
static volatile uint8_t g_key_queue_tail;
static uint8_t g_status_revision = 1U;
static uint8_t g_status_displayed_revision;

/* 两路依次处理，共用这一份“电压换算/FFT 加窗”工作区，避免重复占 4 KB SRAM。 */
static float signal_work_samples[SIGNAL_SAMPLE_COUNT];
static float centered_samples_x[SIGNAL_SAMPLE_COUNT];
static signal_zero_cross_event_t zero_events_x[ZERO_EVENT_CAPACITY];
static float crossing_positions_x[ZERO_EVENT_CAPACITY];
static float mean_v_x;
static float frequency_hz_x;
static float centered_samples_y[SIGNAL_SAMPLE_COUNT];
static signal_zero_cross_event_t zero_events_y[ZERO_EVENT_CAPACITY];
static float crossing_positions_y[ZERO_EVENT_CAPACITY];
static float mean_v_y;
static float frequency_hz_y;

/* X 通道决定公共时间起点；Y 使用同一个窗口，以保留两路真实相位差。 */
typedef struct
{
    float start_sample;
    float end_sample;
} zero_aligned_two_cycle_window_t;
static zero_aligned_two_cycle_window_t g_display_window_x;
static uint8_t g_display_window_x_valid;

static float minimum_v_x;
static float maximum_v_x;
static float vpp_v_x;
static float minimum_v_y;
static float maximum_v_y;
static float vpp_v_y;

static float display_half_range_v=1.0F;

static uint8_t current_page;

static float fft_magnitude[(SIGNAL_SAMPLE_COUNT / 2U) + 1U];
static q15_t fft_q15[2U * SIGNAL_SAMPLE_COUNT];
static float frequency_hz;
static float peak_value;
static float interpolated_bin;
static uint32_t peak_bin;
static signal_fft_parabolic_result_t interpolation_result;
static float thd_percent;
/* 多 bin RSS 换算为正弦峰值幅度：coherent_gain / sqrt(power_gain)。 */
static float fft_harmonic_amplitude_correction = 1.0f;
static signal_harmonic_result_t harmonics;
static signal_thd_result_t thd_result;

static float harmonic_amplitude_x[5U];
static float harmonic_amplitude_y[5U];
static float thd_percent_x;
static float thd_percent_y;
/* MeasureDualChannelHarmonics 返回 bit0=CH1、bit1=CH2。 */
static uint8_t g_harmonics_valid_mask;
/* FFT 基波频率仅供谐波分析页使用；第一页仍显示过零点频率 frequency_hz_x/y。 */
static float fft_frequency_hz_x;
static float fft_frequency_hz_y;

/* 函数索引：App_MapSampleX/MapY 将采样点映射为屏幕像素；App_DrawText/ClearText/
 * DrawInt/DrawFixed4 是局部文字绘制；App_DrawStaticUi 画边框和轴；App_DrawStatus/
 * DrawPhaseStatus 更新数值；App_DrawTrace 画两路曲线；App_MeasurePhase 调用相位模块；
 * App_ProcessKey、队列函数和 SysTick 完成 22_X 风格按键扫描；main 初始化并循环采集。
 * index 是点序号，code 是 ADC 原始码，symbol 是键盘字符，previous_y 保存旧曲线。 */

static void HandlePageSwitch(char key)
{
    if (key == 'C') {
        current_page = (uint8_t)((current_page + 1U) % 2U);
    }
}

static void DrawPage(void)
{
    const char *page_text = (current_page == 0U) ? "PAGE 0" : "PAGE 1";

    (void)TFT_ST7789_DrawString(&g_tft, 240, 2, page_text,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK,
        false, false);
}

static void AutoRange_Update(void){
    float peak=0.0F;
    if(vpp_v_x > vpp_v_y)
    {
        peak = vpp_v_x / 2;
    }
    else {
        peak = vpp_v_y / 2;
    }
    display_half_range_v=fmaxf(0.01F,1.25F*peak);
}

/* 自写：按屏幕曲线点序号映射 X 坐标。采样位置可为浮点数，不能再用它映射 X。 */
static int32_t App_MapSampleX(uint16_t point)
{
    return APP_PLOT_X + (int32_t)(((uint32_t)point * (APP_PLOT_W - 1U)) /
        (APP_TRACE_POINT_COUNT - 1U));
}

/* 自写：按 ADC 码 code 映射 Y 坐标；APP_ADC_MID 决定屏幕中线。 */
static int32_t App_MapY(float voltage_v)
{
    return APP_PLOT_Y + APP_PLOT_H / 2 -
        (int32_t)((voltage_v * (APP_PLOT_H / 2 - 1U)) / display_half_range_v);
}

/* 复用 26_zero_aligned_two_cycle：返回恰好两个周期的 X 通道时间窗口。 */
static bool ZeroAlignedTwoCycle_Find(
    const float *centered_samples,
    uint32_t sample_count,
    signal_zero_cross_event_t *events,
    uint32_t event_capacity,
    float *crossing_positions,
    zero_aligned_two_cycle_window_t *window)
{
    const signal_zero_cross_config_t zero_config = {
        0.0f, 0.005f, SIGNAL_ZERO_CROSS_RISING
    };
    signal_zero_cross_result_t zero_result;
    signal_zero_cross_interpolation_result_t interpolation_result;
    uint32_t index;

    if (SignalZeroCross_Process(centered_samples, sample_count,
            &zero_config, events, event_capacity, &zero_result) !=
            SIGNAL_ALGORITHM_OK || zero_result.rising_count < 4U) {
        return false;
    }
    if (SignalZeroCrossInterpolation_Process(centered_samples, sample_count,
            0.0f, events, zero_result.event_count, crossing_positions,
            event_capacity, &interpolation_result) != SIGNAL_ALGORITHM_OK ||
        interpolation_result.position_count < 4U) {
        return false;
    }
    for (index = 0U; index + 3U < interpolation_result.position_count; ++index) {
        if (crossing_positions[index + 3U] > crossing_positions[index]) {
            window->start_sample = crossing_positions[index];
            window->end_sample = crossing_positions[index + 3U];
            return true;
        }
    }
    return false;
}

/* 在浮点采样位置取值。过零位置来自相邻两点的线性插值，因此边界取值为 0 V。 */
static float App_SampleAtPosition(const float *samples, float position)
{
    uint32_t left_index;
    float fraction;

    if (position <= 0.0f) return samples[0U];
    if (position >= (float)(SIGNAL_SAMPLE_COUNT - 1U)) {
        return samples[SIGNAL_SAMPLE_COUNT - 1U];
    }
    left_index = (uint32_t)position;
    fraction = position - (float)left_index;
    return samples[left_index] + fraction *
        (samples[left_index + 1U] - samples[left_index]);
}

/* 自写包装：在指定位置画文字；实际字库和 SPI 绘制来自 ST7789 模块。 */
static void App_DrawText(int32_t x, int32_t y, const char *text, uint16_t color)
{
    (void)TFT_ST7789_DrawString(&g_tft, x, y, text, TFT_ST7789_FONT_8X16,
        color, TFT_ST7789_BLACK, false, false);
}

/* 自写局部刷新：只清除 character_count 个字符宽度，避免整屏刷新。 */
static void App_ClearText(int32_t x, int32_t y, uint8_t character_count)
{
    (void)TFT_ST7789_FillRect(&g_tft, x, y, character_count * 8, 16,
        TFT_ST7789_BLACK);
}

/* 自写包装：显示整数 value；数值区域由调用者先清除。 */
static void App_DrawInt(int32_t x, int32_t y, int32_t value, uint16_t color)
{
    (void)TFT_ST7789_DrawInt32(&g_tft, x, y, value, TFT_ST7789_FONT_8X16,
        color, TFT_ST7789_BLACK, false);
}

/* 自写包装：把 value 按固定 4 位显示，便于相位/门限数字不残留旧字符。 */
/*static void App_DrawFixed4(int32_t x, int32_t y, uint16_t value, uint16_t color)
{
    char text[5];
    text[0] = (char)('0' + ((value / 1000U) % 10U));
    text[1] = (char)('0' + ((value / 100U) % 10U));
    text[2] = (char)('0' + ((value / 10U) % 10U));
    text[3] = (char)('0' + (value % 10U));
    text[4] = '\0';
    App_DrawText(x, y, text, color);
}*/


/* 自写：上电或翻页时画边框、坐标轴和固定标签；动态循环不重复画。 */
static void App_DrawStaticUi(void)
{
    (void)TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);
    if (current_page == 1U) {
        App_DrawText(8, 8, "HARMONIC MEASURE", TFT_ST7789_CYAN);
        App_DrawText(176, 8, "AMP(V)", TFT_ST7789_WHITE);
        App_DrawText(72, 40, "CH1", TFT_ST7789_YELLOW);
        App_DrawText(192, 40, "CH2", TFT_ST7789_CYAN);
        App_DrawText(8, 64, "FUND", TFT_ST7789_WHITE);
        App_DrawText(8, 88, "H2", TFT_ST7789_WHITE);
        App_DrawText(8, 112, "H3", TFT_ST7789_WHITE);
        App_DrawText(8, 136, "H4", TFT_ST7789_WHITE);
        App_DrawText(8, 160, "H5", TFT_ST7789_WHITE);
        App_DrawText(8, 184, "THD%", TFT_ST7789_WHITE);
        App_DrawText(8, 216, "C: PAGE 0", TFT_ST7789_WHITE);
        return;
    }
    (void)TFT_ST7789_DrawRect(&g_tft, APP_PLOT_X, APP_PLOT_Y,
        APP_PLOT_W, APP_PLOT_H, TFT_ST7789_BLUE);
    (void)TFT_ST7789_DrawLine(&g_tft, APP_PLOT_X + APP_PLOT_W / 2,
        APP_PLOT_Y, APP_PLOT_X + APP_PLOT_W / 2,
        APP_PLOT_Y + APP_PLOT_H - 1, TFT_ST7789_BLUE);
    (void)TFT_ST7789_DrawLine(&g_tft, APP_PLOT_X, APP_PLOT_Y + APP_PLOT_H / 2,
        APP_PLOT_X + APP_PLOT_W - 1, APP_PLOT_Y + APP_PLOT_H / 2,
        TFT_ST7789_BLUE);
    (void)TFT_ST7789_DrawRect(&g_tft, APP_STATUS_X, APP_STATUS_Y,
        APP_STATUS_W, APP_STATUS_H, TFT_ST7789_GREEN);
    App_DrawText(8, 2, "DUAL ADC", TFT_ST7789_CYAN);
    App_DrawText(80, 2, "SYNC", TFT_ST7789_CYAN);
    App_DrawText(96, 2, "Fs=", TFT_ST7789_WHITE);
    App_DrawText(144, 2, "kHz", TFT_ST7789_WHITE);
    App_DrawText(176, 2, "TH=", TFT_ST7789_WHITE);

    App_DrawText(8, 20, "TIME WAVE", TFT_ST7789_CYAN);
    App_DrawText(96, 20, "X:Y", TFT_ST7789_WHITE);
    App_DrawText(136, 20, "PH=", TFT_ST7789_WHITE);

    App_DrawText(240, 44, "FX", TFT_ST7789_WHITE);
    App_DrawText(240, 62, "FY", TFT_ST7789_WHITE);
    App_DrawText(240, 84, "RATE", TFT_ST7789_WHITE);
    App_DrawText(240, 116, "THRESH", TFT_ST7789_WHITE);
    App_DrawText(240, 148, "VppX", TFT_ST7789_WHITE);
    App_DrawText(240, 180, "VppY", TFT_ST7789_WHITE);
    DrawPage();


}

/* 自写：刷新采样率、按键计数等状态字段，只改右侧局部区域。 */
static void App_DrawStatus(void)
{
    App_ClearText(120, 2, 3);
    App_DrawInt(120, 2, (int32_t)(g_sample_rate / 1000U), TFT_ST7789_YELLOW);
    App_ClearText(200, 2, 4);
    App_DrawInt(200, 2, g_threshold, TFT_ST7789_GREEN);

    App_ClearText(168, 20, 6);
    if (g_phase_valid != 0U) App_DrawInt(168, 20, g_phase_degrees, TFT_ST7789_CYAN);
    else App_DrawText(168, 20, "----", TFT_ST7789_MAGENTA);


    App_ClearText(240, 100, 8);
    App_DrawInt(240, 100, (int32_t)(g_sample_rate / 1000U), TFT_ST7789_YELLOW);
    App_DrawText(264, 100, "kHz", TFT_ST7789_YELLOW);
    App_ClearText(240, 134, 8);
    App_DrawInt(240, 134, g_threshold, TFT_ST7789_GREEN);


}

static void APP_DrawVPPStatus(void)
{
    App_ClearText(240, 166, 8);
    (void)TFT_ST7789_DrawFloat(&g_tft, 240, 166, vpp_v_x, 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN,
        TFT_ST7789_BLACK, false);
    App_ClearText(240, 198, 8);
    (void)TFT_ST7789_DrawFloat(&g_tft, 240, 198, vpp_v_y, 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN,
        TFT_ST7789_BLACK, false);

}

/* 自写：显示相位有效性、相位角和测量状态；数据来自 dual_adc_phase 模块。 */
static void App_DrawPhaseStatus(void)
{
    App_ClearText(168, 20, 6);
    if (g_phase_valid != 0U) App_DrawInt(168, 20, g_phase_degrees, TFT_ST7789_CYAN);
    else App_DrawText(168, 20, "----", TFT_ST7789_MAGENTA);
}

/* 自写：将双 ADC 缓冲区映射并绘制 X/Y 波形。 */
static void App_DrawTrace(void)
{
    uint16_t point;
    float start_sample = 0.0f;
    float sample_span = (float)(SIGNAL_SAMPLE_COUNT - 1U);

    if (g_display_window_x_valid != 0U) {
        start_sample = g_display_window_x.start_sample;
        sample_span = g_display_window_x.end_sample - start_sample;
    }
    (void)TFT_ST7789_FillRect(&g_tft, APP_PLOT_X + 1, APP_PLOT_Y + 1,
        APP_PLOT_W - 2, APP_PLOT_H - 2, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawLine(&g_tft, APP_PLOT_X + APP_PLOT_W / 2,
        APP_PLOT_Y + 1, APP_PLOT_X + APP_PLOT_W / 2,
        APP_PLOT_Y + APP_PLOT_H - 2, TFT_ST7789_BLUE);
    (void)TFT_ST7789_DrawLine(&g_tft, APP_PLOT_X + 1,
        APP_PLOT_Y + APP_PLOT_H / 2, APP_PLOT_X + APP_PLOT_W - 2,
        APP_PLOT_Y + APP_PLOT_H / 2, TFT_ST7789_BLUE);
    for (point = 0U; point + 1U < APP_TRACE_POINT_COUNT; ++point) {
        float position0 = start_sample + sample_span * (float)point /
            (float)(APP_TRACE_POINT_COUNT - 1U);
        float position1 = start_sample + sample_span * (float)(point + 1U) /
            (float)(APP_TRACE_POINT_COUNT - 1U);
        (void)TFT_ST7789_DrawLine(&g_tft, App_MapSampleX(point),
            App_MapY(App_SampleAtPosition(centered_samples_x, position0)),
            App_MapSampleX((uint16_t)(point + 1U)),
            App_MapY(App_SampleAtPosition(centered_samples_x, position1)),
            TFT_ST7789_YELLOW);
        (void)TFT_ST7789_DrawLine(&g_tft, App_MapSampleX(point),
            App_MapY(App_SampleAtPosition(centered_samples_y, position0)),
            App_MapSampleX((uint16_t)(point + 1U)),
            App_MapY(App_SampleAtPosition(centered_samples_y, position1)),
            TFT_ST7789_CYAN);
    }
}

/* 模块调用组合：按 README 配置并调用 SIGNAL_DUAL_ADC_PHASE，保存相位结果。 */
static void App_MeasurePhase(void)
{
    signal_dual_adc_phase_result_t result;
    signal_algorithm_status_t status;

    g_phase_valid = 0U;
    /* 【双路同步 ADC 相位模块】模块内部完成去中心、迟滞过零、周期换算和相位归一化；
     * main 只提供原始双路数组、采样率和 g_phase_config。 */
    status = SignalDualADCPhase_Process(
        g_raw_x, g_raw_y, SIGNAL_SAMPLE_COUNT, g_sample_rate,
        &g_phase_config, &result);
    if ((status == SIGNAL_ALGORITHM_OK) && (result.valid != 0U)) {
        g_phase_degrees = (int32_t)result.phase_degrees;
        g_phase_valid = 1U;
    }

    if (++g_phase_display_divider >= 8U) {
        g_phase_display_divider = 0U;
        App_DrawPhaseStatus();
    }
}

/* 自写状态机：根据 symbol 切换页面、门限或测量通道；不在中断中调用。 */
static void App_ProcessKey(char symbol)
{
    if (symbol == 'C') {
        HandlePageSwitch(symbol);
        App_DrawStaticUi();
        if (current_page == 0U) {
            App_DrawStatus();
            g_status_displayed_revision = g_status_revision;
        }
        return;
    }
    if (symbol == 'A') {
        g_threshold = (uint16_t)(g_threshold > 64U ? g_threshold - 64U : 64U);
    } else if (symbol == 'B') {
        g_threshold = (uint16_t)(g_threshold < 4031U ? g_threshold + 64U : 4031U);
    } else if (symbol == 'D') {
        g_sample_rate = (g_sample_rate == 100000U) ? 50000U : 100000U;
        /* 【双 ADC 模块】按键改变采样率后，由模块重算触发定时器。 */
        (void)SignalDualADC_SetSampleRate(g_sample_rate);
    }

    ++g_status_revision;
}

/* 自写：主循环逐个取出环形队列中的按键并调用 App_ProcessKey。 */
static void App_ProcessQueuedKeys(void)
{
    char symbol;

    while (g_key_queue_tail != g_key_queue_head) {
        symbol = g_key_queue[g_key_queue_tail];
        g_key_queue_tail = (uint8_t)((g_key_queue_tail + 1U) %
            APP_KEY_QUEUE_SIZE);
        App_ProcessKey(symbol);
    }
}

/* 自写：把 SysTick 扫到的稳定按键写入队列；队列满时丢弃新键保护旧数据。 */
static void App_QueueKey(char symbol)
{
    uint8_t next = (uint8_t)((g_key_queue_head + 1U) %
        APP_KEY_QUEUE_SIZE);
    if (next != g_key_queue_tail) {
        g_key_queue[g_key_queue_head] = symbol;
        g_key_queue_head = next;
    }
}

/* 自写中断：按 5 ms 周期调用键盘模块，只入队，不执行屏幕和算法。 */
void SysTick_Handler(void)
{
    static uint8_t milliseconds;
    char symbol;

    ++milliseconds;
    if (milliseconds < APP_KEYPAD_SCAN_PERIOD_MS) return;
    milliseconds = 0U;

    /* 【矩阵键盘模块】固定 GPIO 扫描、消抖和鬼键过滤；本函数只接收 symbol。 */
    g_keypad_status = SignalMatrixKeypad4x4_ReadNewSymbol(&symbol);
    if (g_keypad_status == SIGNAL_RESULT_OK) {
        App_QueueKey(symbol);
    }
}

/* ============================================================
 * fuyong 统一数据接口别名层
 *
 * 新增 fuyong 教学 static 函数时，请把函数放在本块之后、main() 之前；即可直接
 * 使用以下标准名称，不需要把本题既有 g_* 存储重命名。两路数组均为 uint16_t ADC
 * code，sample_rate_hz 为 Hz，phase_deg 为 deg。
 * 本工程的 DMA 在主循环内同步等待完成，未额外导出 adc_frame_ready。
 * ============================================================ */
#define SAMPLE_COUNT       SIGNAL_SAMPLE_COUNT
#define adc_ch1_samples    g_raw_x
#define adc_ch2_samples    g_raw_y
#define sample_rate_hz     g_sample_rate
#define phase_deg          g_phase_degrees




static void PrepareSignalX(void)
{
    uint32_t index;
    uint32_t ignored_index;

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        signal_work_samples[index] = (float)adc_ch1_samples[index] *
            SIGNAL_ADC_VREF_V / 4095.0f;
    }

    arm_mean_f32(signal_work_samples, SIGNAL_SAMPLE_COUNT, &mean_v_x);
    arm_offset_f32(signal_work_samples, -mean_v_x, centered_samples_x,
        SIGNAL_SAMPLE_COUNT);
    arm_min_f32(signal_work_samples, SIGNAL_SAMPLE_COUNT, &minimum_v_x,
        &ignored_index);
    arm_max_f32(signal_work_samples, SIGNAL_SAMPLE_COUNT, &maximum_v_x,
        &ignored_index);
    vpp_v_x = maximum_v_x - minimum_v_x;
}

static void PrepareSignalY(void)
{
    uint32_t index;
    uint32_t ignored_index;

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        signal_work_samples[index] = (float)adc_ch2_samples[index] *
            SIGNAL_ADC_VREF_V / 4095.0f;
    }

    arm_mean_f32(signal_work_samples, SIGNAL_SAMPLE_COUNT, &mean_v_y);
    arm_offset_f32(signal_work_samples, -mean_v_y, centered_samples_y,
        SIGNAL_SAMPLE_COUNT);
    arm_min_f32(signal_work_samples, SIGNAL_SAMPLE_COUNT, &minimum_v_y,
        &ignored_index);
    arm_max_f32(signal_work_samples, SIGNAL_SAMPLE_COUNT, &maximum_v_y,
        &ignored_index);
    vpp_v_y = maximum_v_y - minimum_v_y;
}

static bool MeasureFrequencyZeroCrossX(void)
{
    const signal_zero_cross_config_t zero_config = {
        0.0f, 0.005f, SIGNAL_ZERO_CROSS_RISING
    };
    signal_zero_cross_result_t zero_result;
    signal_zero_cross_interpolation_result_t interpolation_result;
    float sample_distance;

    if (SignalZeroCross_Process(centered_samples_x, SIGNAL_SAMPLE_COUNT,
            &zero_config, zero_events_x, ZERO_EVENT_CAPACITY,
            &zero_result) != SIGNAL_ALGORITHM_OK ||
        zero_result.rising_count < 2U) {
        return false;
    }

    if (SignalZeroCrossInterpolation_Process(centered_samples_x,
            SIGNAL_SAMPLE_COUNT, 0.0f, zero_events_x,
            zero_result.event_count, crossing_positions_x,
            ZERO_EVENT_CAPACITY, &interpolation_result) !=
            SIGNAL_ALGORITHM_OK ||
        interpolation_result.position_count < 2U) {
        return false;
    }

    sample_distance = crossing_positions_x[interpolation_result.position_count - 1U] -
        crossing_positions_x[0U];
    if (sample_distance <= 0.0f) {
        return false;
    }

    frequency_hz_x = ((float)(interpolation_result.position_count - 1U) *
        sample_rate_hz) / sample_distance;
    return true;
}

static bool MeasureFrequencyZeroCrossY(void)
{
    const signal_zero_cross_config_t zero_config = {
        0.0f, 0.005f, SIGNAL_ZERO_CROSS_RISING
    };
    signal_zero_cross_result_t zero_result;
    signal_zero_cross_interpolation_result_t interpolation_result;
    float sample_distance;

    if (SignalZeroCross_Process(centered_samples_y, SIGNAL_SAMPLE_COUNT,
            &zero_config, zero_events_y, ZERO_EVENT_CAPACITY,
            &zero_result) != SIGNAL_ALGORITHM_OK ||
        zero_result.rising_count < 2U) {
        return false;
    }

    if (SignalZeroCrossInterpolation_Process(centered_samples_y,
            SIGNAL_SAMPLE_COUNT, 0.0f, zero_events_y,
            zero_result.event_count, crossing_positions_y,
            ZERO_EVENT_CAPACITY, &interpolation_result) !=
            SIGNAL_ALGORITHM_OK ||
        interpolation_result.position_count < 2U) {
        return false;
    }

    sample_distance = crossing_positions_y[interpolation_result.position_count - 1U] -
        crossing_positions_y[0U];
    if (sample_distance <= 0.0f) {
        return false;
    }

    frequency_hz_y = ((float)(interpolation_result.position_count - 1U) *
        sample_rate_hz) / sample_distance;
    return true;
}

static void App_DrawFrequencyStatus(void)
{
    App_ClearText(264, 44, 7);
    (void)TFT_ST7789_DrawFloat(&g_tft, 264, 44,
        frequency_hz_x / 1000.0f, 1U,
        TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW,
        TFT_ST7789_BLACK, false);
    App_DrawText(296, 44, "kHz", TFT_ST7789_YELLOW);

    App_ClearText(264, 62, 7);
    (void)TFT_ST7789_DrawFloat(&g_tft, 264, 62,
        frequency_hz_y / 1000.0f, 1U,
        TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN,
        TFT_ST7789_BLACK, false);
    App_DrawText(296, 62, "kHz", TFT_ST7789_CYAN);
}

/* 第二页仅负责显示：CH1/CH2 的数值由 FFT 谐波分析函数每帧更新。 */
static void App_DrawHarmonicPageValues(void)
{
    static const int32_t rows[] = {64, 88, 112, 136, 160, 184};
    const float values_x[] = {
        harmonic_amplitude_x[0], harmonic_amplitude_x[1],
        harmonic_amplitude_x[2], harmonic_amplitude_x[3],
        harmonic_amplitude_x[4], thd_percent_x
    };
    const float values_y[] = {
        harmonic_amplitude_y[0], harmonic_amplitude_y[1],
        harmonic_amplitude_y[2], harmonic_amplitude_y[3],
        harmonic_amplitude_y[4], thd_percent_y
    };
    uint32_t index;

    for (index = 0U; index < 6U; ++index) {
        App_ClearText(56, rows[index], 12U);
        App_ClearText(176, rows[index], 12U);
        if ((g_harmonics_valid_mask & 0x01U) != 0U) {
            (void)TFT_ST7789_DrawFloat(&g_tft, 56, rows[index],
                values_x[index], 3U, TFT_ST7789_FONT_8X16,
                TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
        } else {
            App_DrawText(56, rows[index], "----", TFT_ST7789_MAGENTA);
        }
        if ((g_harmonics_valid_mask & 0x02U) != 0U) {
            (void)TFT_ST7789_DrawFloat(&g_tft, 176, rows[index],
                values_y[index], 3U, TFT_ST7789_FONT_8X16,
                TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
        } else {
            App_DrawText(176, rows[index], "----", TFT_ST7789_MAGENTA);
        }
    }
}

static bool RunQ15FFTLowRam(const float *input)
{
    const arm_cfft_instance_q15 *instance = &arm_cfft_sR_q15_len1024;
    uint32_t index;
    float max_abs = 0.0f;
    float restore_scale;

#if SIGNAL_SAMPLE_COUNT == 512U
    instance = &arm_cfft_sR_q15_len512;
#elif SIGNAL_SAMPLE_COUNT != 1024U
#error "20_fft_analysis currently documents the verified 512/1024 Q15 CMSIS sizes"
#endif

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        const float abs_value = fabsf(input[index]);
        if (abs_value > max_abs) {
            max_abs = abs_value;
        }
    }
    if (max_abs == 0.0f) {
        arm_fill_f32(0.0f, fft_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U);
        return true;
    }

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        fft_q15[2U * index] =
            (q15_t)((input[index] / max_abs) * 32767.0f);
        fft_q15[2U * index + 1U] = 0;
    }
    arm_cfft_q15(instance, fft_q15, 0U, 1U);

    /* CMSIS Q15 复数幅值是 Q2.14，数值 1.0 对应 16384。 */
    restore_scale = max_abs * (float)SIGNAL_SAMPLE_COUNT / 16384.0f;
    for (index = 0U; index <= SIGNAL_SAMPLE_COUNT / 2U; ++index) {
        q15_t real = fft_q15[2U * index];
        q15_t imag = fft_q15[2U * index + 1U];
        q31_t magnitude_q31;
        uint32_t magnitude_square =
            (((uint32_t)((q31_t)real * real) +
              (uint32_t)((q31_t)imag * imag)) >> 1U);

        if (arm_sqrt_q31((q31_t)magnitude_square,
                &magnitude_q31) != ARM_MATH_SUCCESS) {
            return false;
        }
        fft_magnitude[index] =
            (float)(magnitude_q31 >> 16U) * restore_scale;
    }
    return true;
}

static bool RunFFTCommonLowRam(const float *centered_input, float *fft_input)
{
    signal_window_result_t window_result;

    if (SignalWindow_Apply(centered_input, fft_input,
            SIGNAL_SAMPLE_COUNT, SIGNAL_WINDOW_HANN,
            &window_result) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    if (!RunQ15FFTLowRam(fft_input)) {
        return false;
    }
    if (SignalWindowGainCorrection_Apply(fft_magnitude, fft_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U, SIGNAL_SAMPLE_COUNT,
            window_result.coherent_gain) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    fft_harmonic_amplitude_correction = window_result.coherent_gain /
        sqrtf(window_result.power_gain);
    return isfinite(fft_harmonic_amplitude_correction) &&
        (fft_harmonic_amplitude_correction > 0.0f);
}

static bool MeasureFFTFrequency(void)
{
    arm_max_f32(&fft_magnitude[1], SIGNAL_SAMPLE_COUNT / 2U,
        &peak_value, &peak_bin);
    peak_bin += 1U;
    if (peak_value <= 0.0f) {
        return false;
    }
    frequency_hz = (float)peak_bin * sample_rate_hz /
        (float)SIGNAL_SAMPLE_COUNT;
    return true;
}

static bool RefineFFTFrequency(void)
{
    if (SignalFFTParabolicInterpolation_Process(fft_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U, peak_bin, sample_rate_hz,
            SIGNAL_SAMPLE_COUNT, &interpolation_result) !=
            SIGNAL_ALGORITHM_OK) {
        return false;
    }
    interpolated_bin = interpolation_result.fractional_bin;
    frequency_hz = interpolation_result.frequency_hz;
    return true;
}

static bool AnalyzeHarmonicsAndTHDToOrder(uint32_t last_order)
{
    const signal_harmonic_config_t harmonic_config = {
        frequency_hz, 1U, last_order, 1U
    };

    if (SignalHarmonic_Process(fft_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U, sample_rate_hz,
            SIGNAL_SAMPLE_COUNT, &harmonic_config,
            &harmonics) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    if (SignalTHD_Process(&harmonics, &thd_result) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    thd_percent = thd_result.thd_percent;
    return true;
}

static bool MeasureChannelHarmonics(
    const float *centered_input,
    float *fft_input,
    uint32_t last_order,
    float *fundamental_frequency_hz,
    float *harmonic_amplitude_v,
    float *thd_percent_out)
{
    uint32_t order;

    if ((last_order < 2U) || (last_order > SIGNAL_HARMONIC_MAX_ORDER)) {
        return false;
    }
    if (!RunFFTCommonLowRam(centered_input, fft_input) ||
        !MeasureFFTFrequency() || !RefineFFTFrequency() ||
        !AnalyzeHarmonicsAndTHDToOrder(last_order)) {
        return false;
    }

    *fundamental_frequency_hz = frequency_hz;
    for (order = 1U; order <= last_order; ++order) {
        harmonic_amplitude_v[order - 1U] =
            harmonics.items[order].root_sum_square *
            fft_harmonic_amplitude_correction;
    }
    *thd_percent_out = thd_percent;
    return true;
}

static uint8_t MeasureDualChannelHarmonics(
    const float *centered_x,
    float *fft_input_x,
    const float *centered_y,
    float *fft_input_y,
    uint32_t last_order,
    float *fundamental_frequency_x_hz,
    float *harmonic_amplitude_x_v,
    float *thd_x_percent,
    float *fundamental_frequency_y_hz,
    float *harmonic_amplitude_y_v,
    float *thd_y_percent)
{
    uint8_t valid_mask = 0U;

    if (MeasureChannelHarmonics(centered_x, fft_input_x, last_order,
            fundamental_frequency_x_hz, harmonic_amplitude_x_v,
            thd_x_percent)) {
        valid_mask |= 0x01U;
    }
    if (MeasureChannelHarmonics(centered_y, fft_input_y, last_order,
            fundamental_frequency_y_hz, harmonic_amplitude_y_v,
            thd_y_percent)) {
        valid_mask |= 0x02U;
    }
    return valid_mask;
}



/* 在这里粘贴 fuyong 的 static COPY 函数；它们可直接使用上面的统一名称。 */
#undef SAMPLE_COUNT
#undef adc_ch1_samples
#undef adc_ch2_samples
#undef sample_rate_hz
#undef phase_deg
#undef current_page

/* main：初始化双 ADC、相位模块、键盘和 ST7789；每次 DMA 完成后画 X/Y 轨迹并更新
 * 相位状态。按键先入队，主循环再处理，避免屏幕 SPI 操作阻塞固定扫描节拍。 */
int main(void)
{
    const signal_dual_adc_config_t adc_config = {
        SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
    };
    SYSCFG_DL_init();
    /* 【双 ADC 模块】按 README 初始化同步触发、DMA 通道和完成中断。 */
    g_adc_status = SignalDualADC_Init(&adc_config);
    if (g_adc_status != SIGNAL_RESULT_OK) while (1) { }
    if (SignalTFTST7789_MSPM0_Init(&g_tft, TFT_ST7789_ROTATION_270,
        0U, 0U) != TFT_ST7789_OK) while (1) { }
    App_DrawStaticUi();
    App_DrawStatus();
    g_status_displayed_revision = g_status_revision;
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (1) { }

    while (1) {
        App_ProcessQueuedKeys();
        if ((current_page == 0U) &&
            (g_status_displayed_revision != g_status_revision)) {
            App_DrawStatus();
            g_status_displayed_revision = g_status_revision;
        }
        /* 【双 ADC 模块】启动一次同步采样帧。 */
        g_adc_status = SignalDualADC_Start(g_raw_x, g_raw_y,
            SIGNAL_SAMPLE_COUNT);
        if (g_adc_status != SIGNAL_RESULT_OK) continue;
        while (!SignalDualADC_IsFinished()) { __WFI(); }

        PrepareSignalY();
        PrepareSignalX();

        if (current_page == 0U) {
            /* 第一页只执行原来的波形、过零点测频和相位测量。 */
            g_display_window_x_valid = (uint8_t)ZeroAlignedTwoCycle_Find(
                centered_samples_x, SIGNAL_SAMPLE_COUNT, zero_events_x,
                ZERO_EVENT_CAPACITY, crossing_positions_x,
                &g_display_window_x);
            (void)MeasureFrequencyZeroCrossY();
            (void)MeasureFrequencyZeroCrossX();
            AutoRange_Update();
            App_DrawTrace();
            App_MeasurePhase();
            App_DrawFrequencyStatus();
            APP_DrawVPPStatus();
        } else {
            /* 第二页才执行 FFT；两路顺序处理并共用 signal_work_samples。 */
            g_harmonics_valid_mask = MeasureDualChannelHarmonics(
                centered_samples_x, signal_work_samples,
                centered_samples_y, signal_work_samples,
                5U,
                &fft_frequency_hz_x, harmonic_amplitude_x, &thd_percent_x,
                &fft_frequency_hz_y, harmonic_amplitude_y, &thd_percent_y);
            App_DrawHarmonicPageValues();
        }
        
    }
}
