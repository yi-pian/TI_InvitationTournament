/* example01 精简闭环：双 ADC 采样、ST7789 波形显示和双路相位差测量。
 * 模块 API 按 README 复制；App_* 只负责坐标映射、键盘队列、页面和显示逻辑。
 * 默认显示范围、颜色和位置在 APP_* 宏中改，矩阵键盘 GPIO 在模块固定配置中改。 */
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_dual_adc_phase.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"

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

/* g_raw_x/g_raw_y：X、Y 两路 ADC 原始码；g_sample_rate：当前采样率；g_threshold：
 * 过零门限；g_phase_*：相位测量结果和有效标志；g_key_queue/head/tail：按键环形队列；
 * revision 变量用于数值变化时局部刷新。 */
static uint16_t g_raw_x[SIGNAL_SAMPLE_COUNT];
static uint16_t g_raw_y[SIGNAL_SAMPLE_COUNT];
static tft_st7789_t g_tft;
static uint8_t g_page;
static uint32_t g_sample_rate = SIGNAL_SAMPLE_RATE_HZ;
static uint16_t g_threshold = APP_ADC_MID;
static uint16_t g_key_event_count;
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

/* 函数索引：App_MapSampleX/MapY 将采样点映射为屏幕像素；App_DrawText/ClearText/
 * DrawInt/DrawFixed4 是局部文字绘制；App_DrawStaticUi 画边框和轴；App_DrawStatus/
 * DrawPhaseStatus 更新数值；App_DrawTrace 画两路曲线；App_MeasurePhase 调用相位模块；
 * App_ProcessKey、队列函数和 SysTick 完成 22_X 风格按键扫描；main 初始化并循环采集。
 * index 是点序号，code 是 ADC 原始码，symbol 是键盘字符，previous_y 保存旧曲线。 */
/* 自写：按样本序号 index 映射曲线 X 坐标；修改 APP_PLOT_X/W 可改变绘图区。 */
static int32_t App_MapSampleX(uint16_t index)
{
    return APP_PLOT_X + (int32_t)(((uint32_t)index * (APP_PLOT_W - 1U)) /
        (SIGNAL_SAMPLE_COUNT - 1U));
}

/* 自写：按 ADC 码 code 映射 Y 坐标；APP_ADC_MID 决定屏幕中线。 */
static int32_t App_MapY(uint16_t code)
{
    return APP_PLOT_Y + APP_PLOT_H - 1 -
        (int32_t)(((uint32_t)code * (APP_PLOT_H - 1U)) / 4095U);
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
static void App_DrawFixed4(int32_t x, int32_t y, uint16_t value, uint16_t color)
{
    char text[5];
    text[0] = (char)('0' + ((value / 1000U) % 10U));
    text[1] = (char)('0' + ((value / 100U) % 10U));
    text[2] = (char)('0' + ((value / 10U) % 10U));
    text[3] = (char)('0' + (value % 10U));
    text[4] = '\0';
    App_DrawText(x, y, text, color);
}

/* 自写：上电或翻页时画边框、坐标轴和固定标签；动态循环不重复画。 */
static void App_DrawStaticUi(void)
{
    (void)TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);
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
    App_DrawText(240, 2, "KEY=", TFT_ST7789_WHITE);
    App_DrawText(8, 20, "TIME WAVE", TFT_ST7789_CYAN);
    App_DrawText(96, 20, "X:Y", TFT_ST7789_WHITE);
    App_DrawText(136, 20, "PH=", TFT_ST7789_WHITE);
    App_DrawText(224, 20, "PG=", TFT_ST7789_WHITE);
    App_DrawText(240, 44, "CH1 X", TFT_ST7789_WHITE);
    App_DrawText(240, 62, "CH2 Y", TFT_ST7789_WHITE);
    App_DrawText(240, 84, "RATE", TFT_ST7789_WHITE);
    App_DrawText(240, 116, "THRESH", TFT_ST7789_WHITE);
    App_DrawText(240, 156, "KEY CNT", TFT_ST7789_WHITE);
    App_DrawText(240, 190, "PAGE", TFT_ST7789_WHITE);
}

/* 自写：刷新采样率、按键计数等状态字段，只改右侧局部区域。 */
static void App_DrawStatus(void)
{
    App_ClearText(120, 2, 3);
    App_DrawInt(120, 2, (int32_t)(g_sample_rate / 1000U), TFT_ST7789_YELLOW);
    App_ClearText(200, 2, 4);
    App_DrawInt(200, 2, g_threshold, TFT_ST7789_GREEN);
    App_ClearText(280, 2, 4);
    App_DrawFixed4(280, 2, g_key_event_count, TFT_ST7789_YELLOW);
    App_ClearText(168, 20, 6);
    if (g_phase_valid != 0U) App_DrawInt(168, 20, g_phase_degrees, TFT_ST7789_CYAN);
    else App_DrawText(168, 20, "----", TFT_ST7789_MAGENTA);
    App_ClearText(248, 20, 2);
    App_DrawInt(248, 20, (int32_t)g_page + 1, TFT_ST7789_YELLOW);

    App_ClearText(240, 100, 8);
    App_DrawInt(240, 100, (int32_t)(g_sample_rate / 1000U), TFT_ST7789_YELLOW);
    App_DrawText(264, 100, "kHz", TFT_ST7789_YELLOW);
    App_ClearText(240, 134, 8);
    App_DrawInt(240, 134, g_threshold, TFT_ST7789_GREEN);
    App_ClearText(240, 174, 8);
    App_DrawFixed4(240, 174, g_key_event_count, TFT_ST7789_YELLOW);
    App_ClearText(240, 206, 8);
    App_DrawInt(240, 206, (int32_t)g_page + 1, TFT_ST7789_YELLOW);
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
    (void)TFT_ST7789_FillRect(&g_tft, APP_PLOT_X + 1, APP_PLOT_Y + 1,
        APP_PLOT_W - 2, APP_PLOT_H - 2, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawLine(&g_tft, APP_PLOT_X + APP_PLOT_W / 2,
        APP_PLOT_Y + 1, APP_PLOT_X + APP_PLOT_W / 2,
        APP_PLOT_Y + APP_PLOT_H - 2, TFT_ST7789_BLUE);
    (void)TFT_ST7789_DrawLine(&g_tft, APP_PLOT_X + 1,
        APP_PLOT_Y + APP_PLOT_H / 2, APP_PLOT_X + APP_PLOT_W - 2,
        APP_PLOT_Y + APP_PLOT_H / 2, TFT_ST7789_BLUE);
    for (point = 0U; point + 1U < APP_TRACE_POINT_COUNT; ++point) {
        uint16_t index0 = (uint16_t)(((uint32_t)point *
            (SIGNAL_SAMPLE_COUNT - 1U)) / (APP_TRACE_POINT_COUNT - 1U));
        uint16_t index1 = (uint16_t)(((uint32_t)(point + 1U) *
            (SIGNAL_SAMPLE_COUNT - 1U)) / (APP_TRACE_POINT_COUNT - 1U));
        (void)TFT_ST7789_DrawLine(&g_tft, App_MapSampleX(index0),
            App_MapY(g_raw_x[index0]), App_MapSampleX(index1),
            App_MapY(g_raw_x[index1]), TFT_ST7789_YELLOW);
        (void)TFT_ST7789_DrawLine(&g_tft, App_MapSampleX(index0),
            App_MapY(g_raw_y[index0]), App_MapSampleX(index1),
            App_MapY(g_raw_y[index1]), TFT_ST7789_CYAN);
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
    if ((symbol >= '1') && (symbol <= '5')) {
        g_page = (uint8_t)(symbol - '1');
    } else if (symbol == 'A') {
        g_threshold = (uint16_t)(g_threshold > 64U ? g_threshold - 64U : 64U);
    } else if (symbol == 'B') {
        g_threshold = (uint16_t)(g_threshold < 4031U ? g_threshold + 64U : 4031U);
    } else if (symbol == 'D') {
        g_sample_rate = (g_sample_rate == 100000U) ? 50000U : 100000U;
        /* 【双 ADC 模块】按键改变采样率后，由模块重算触发定时器。 */
        (void)SignalDualADC_SetSampleRate(g_sample_rate);
    }
    ++g_key_event_count;
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
        if (g_status_displayed_revision != g_status_revision) {
            App_DrawStatus();
            g_status_displayed_revision = g_status_revision;
        }
        /* 【双 ADC 模块】启动一次同步采样帧。 */
        g_adc_status = SignalDualADC_Start(g_raw_x, g_raw_y,
            SIGNAL_SAMPLE_COUNT);
        if (g_adc_status != SIGNAL_RESULT_OK) continue;
        while (!SignalDualADC_IsFinished()) { __WFI(); }
        App_DrawTrace();
        App_MeasurePhase();
        (void)g_page;
    }
}
