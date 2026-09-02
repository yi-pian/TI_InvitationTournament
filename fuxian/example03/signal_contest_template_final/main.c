/* example03：双路波形检测与显示。固定边框、XY 格子和单位长度只画一次，
 * 动态部分擦除旧曲线再画新曲线；ADC/去直流/键盘/ST7789 是模块 README 调用，
 * 坐标映射、网格、按键队列和旧曲线恢复属于 main 自写组合逻辑。 */
#include <stdint.h>
#include "arm_math.h"

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"

#define APP_SAMPLE_COUNT       (256U)
#define APP_PLOT_X             (8)
#define APP_PLOT_Y             (40)
#define APP_PLOT_W             (300)
#define APP_PLOT_H             (164)
#define APP_TRACE_POINTS       (220U)
#define APP_KEYPAD_SCAN_MS     (5U)
#define APP_KEY_QUEUE_SIZE     (8U)
#define APP_Y_RANGE_COUNT      (7U)
#define APP_X_RANGE_COUNT      (4U)
#define APP_DISPLAY_HOLD_MS    (500U)

/* g_y_range_tenth/g_x_span_us：可选的 Y 轴半量程和 X 轴时间跨度；g_raw_a/b：双路
 * 原始采样；g_display_a/b：去直流后的显示电压；g_previous_y_*：上一帧曲线像素；
 * g_y_range_index/g_x_range_index：当前档位；g_display_mode：显示通道；队列和
 * g_display_due 控制 5 ms 键盘扫描与 500 ms 波形保持。 */
static const uint16_t g_y_range_tenth[APP_Y_RANGE_COUNT] =
    {5U, 10U, 15U, 20U, 25U, 30U, 35U};
static const uint32_t g_x_span_us[APP_X_RANGE_COUNT] =
    {1280U, 2560U, 5120U, 10240U};
static uint16_t g_raw_a[APP_SAMPLE_COUNT];
static uint16_t g_raw_b[APP_SAMPLE_COUNT];
static float32_t g_display_a[APP_SAMPLE_COUNT];
static float32_t g_display_b[APP_SAMPLE_COUNT];
static int32_t g_previous_y_a[APP_TRACE_POINTS];
static int32_t g_previous_y_b[APP_TRACE_POINTS];
static tft_st7789_t g_tft;
static uint8_t g_y_range_index = 6U;
static uint8_t g_x_range_index = 1U;
static uint8_t g_display_mode = 3U;
static volatile char g_key_queue[APP_KEY_QUEUE_SIZE];
static volatile uint8_t g_key_head;
static volatile uint8_t g_key_tail;
static uint8_t g_previous_a_valid;
static uint8_t g_previous_b_valid;
static volatile uint16_t g_display_elapsed_ms;
static volatile uint8_t g_display_due = 1U;

/* 函数索引：App_MapX/MapY 按 X/Y 量程换算像素；DrawText/DrawStatus 更新文字；
 * DrawGrid/DrawGridUnits/DrawStaticUi 只画一次坐标背景；DrawChannel 画一路波形；
 * BackgroundColor/RestoreLine/ErasePreviousChannel 负责擦除旧线且恢复网格；
 * DrawWaveforms 刷新双路曲线；Queue/Handle/ProcessKeys/SysTick 是键盘队列；main
 * 负责 ADC 采样和循环。centered_voltage 是去直流后的电压，x_unit/y_unit 是每格单位，
 * previous_y 保存上帧像素，避免整屏刷新。 */
/* 自写：按点号和当前 X 时间跨度计算像素 X；改 g_x_span_us 可改变横轴单位。 */
static int32_t App_MapX(uint16_t point)
{
    return APP_PLOT_X + 1 + (int32_t)(((uint32_t)point *
        (APP_PLOT_W - 3U)) / (APP_TRACE_POINTS - 1U));
}

/* 自写：按去直流电压和当前 Y 半量程计算像素 Y；改 g_y_range_index 改纵轴范围。 */
static int32_t App_MapY(float32_t centered_voltage)
{
    int32_t center = APP_PLOT_Y + APP_PLOT_H / 2;
    int32_t half_height = APP_PLOT_H / 2 - 2;
    float32_t range_voltage = (float32_t)g_y_range_tenth[g_y_range_index] / 10.0F;
    int32_t y = center - (int32_t)(centered_voltage * (float32_t)half_height /
        range_voltage);
    if (y < APP_PLOT_Y + 1) y = APP_PLOT_Y + 1;
    if (y > APP_PLOT_Y + APP_PLOT_H - 2) {
        y = APP_PLOT_Y + APP_PLOT_H - 2;
    }
    return y;
}

/* 自写包装：画 8x16 文字；底层 ST7789 DrawString 来自模块。 */
static void App_DrawText(int32_t x, int32_t y, const char *text,
    uint16_t color)
{
    (void)TFT_ST7789_DrawString(&g_tft, x, y, text,
        TFT_ST7789_FONT_8X16, color, TFT_ST7789_BLACK, false, false);
}

/* 自写：局部更新当前通道、门限、X/Y 单位等状态文字。 */
static void App_DrawStatus(void)
{
    float range = (float)g_y_range_tenth[g_y_range_index] / 10.0F;
    uint32_t span = g_x_span_us[g_x_range_index];
    (void)TFT_ST7789_FillRect(&g_tft, 8, 20, 304, 16, TFT_ST7789_BLACK);
    App_DrawText(8, 20, (g_display_mode == 1U) ? "CH1" :
        ((g_display_mode == 2U) ? "CH2" : "CH1+CH2"), TFT_ST7789_CYAN);
    App_DrawText(72, 20, "Y:+/-", TFT_ST7789_WHITE);
    (void)TFT_ST7789_DrawFloat(&g_tft, 120, 20, range, 1U,
        TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    App_DrawText(144, 20, "V X:", TFT_ST7789_WHITE);
    (void)TFT_ST7789_DrawInt32(&g_tft, 184, 20, (int32_t)span,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    App_DrawText(248, 20, "us", TFT_ST7789_WHITE);
}

/* 自写：按当前量程画 XY 网格，只有量程改变时重新绘制。 */
static void App_DrawGrid(void)
{
    uint8_t division;
    int32_t x;
    int32_t y;
    const uint16_t grid_color = TFT_ST7789_RGB565(55U, 75U, 85U);
    for (division = 1U; division < 5U; ++division) {
        x = APP_PLOT_X + 1 + (int32_t)(((uint32_t)division *
            (APP_PLOT_W - 3U)) / 5U);
        (void)TFT_ST7789_DrawLine(&g_tft, x, APP_PLOT_Y + 1,
            x, APP_PLOT_Y + APP_PLOT_H - 2, grid_color);
    }
    for (division = 1U; division < 4U; ++division) {
        y = APP_PLOT_Y + 1 + (int32_t)(((uint32_t)division *
            (APP_PLOT_H - 3U)) / 4U);
        (void)TFT_ST7789_DrawLine(&g_tft, APP_PLOT_X + 1, y,
            APP_PLOT_X + APP_PLOT_W - 2, y, grid_color);
    }
}

/* 自写：在网格旁显示每格对应的电压和时间单位。 */
static void App_DrawGridUnits(void)
{
    float y_div = (float)g_y_range_tenth[g_y_range_index] / 20.0F;
    float x_div = (float)g_x_span_us[g_x_range_index] / 5.0F;
    (void)TFT_ST7789_FillRect(&g_tft, 8, 214, 304, 18,
        TFT_ST7789_BLACK);
    App_DrawText(8, 215, "YDIV:", TFT_ST7789_WHITE);
    (void)TFT_ST7789_DrawFloat(&g_tft, 48, 215, y_div, 2U,
        TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    App_DrawText(88, 215, "V XDIV:", TFT_ST7789_WHITE);
    (void)TFT_ST7789_DrawFloat(&g_tft, 144, 215, x_div, 1U,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    App_DrawText(192, 215, "us", TFT_ST7789_WHITE);
}

/* 自写：画固定边框、标题、网格和单位；波形刷新不调用整屏清除。 */
static void App_DrawStaticUi(void)
{
    (void)TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);
    App_DrawText(8, 4, "DUAL WAVEFORM", TFT_ST7789_CYAN);
    App_DrawText(176, 4, "A/B:Y C/D:X", TFT_ST7789_WHITE);
    (void)TFT_ST7789_DrawRect(&g_tft, APP_PLOT_X, APP_PLOT_Y,
        APP_PLOT_W, APP_PLOT_H, TFT_ST7789_BLUE);
    (void)TFT_ST7789_DrawLine(&g_tft, APP_PLOT_X + 1,
        APP_PLOT_Y + APP_PLOT_H / 2, APP_PLOT_X + APP_PLOT_W - 2,
        APP_PLOT_Y + APP_PLOT_H / 2, TFT_ST7789_BLUE);
    (void)TFT_ST7789_DrawLine(&g_tft, APP_PLOT_X + APP_PLOT_W / 2,
        APP_PLOT_Y + 1, APP_PLOT_X + APP_PLOT_W / 2,
        APP_PLOT_Y + APP_PLOT_H - 2, TFT_ST7789_BLUE);
    App_DrawGrid();
    App_DrawStatus();
    App_DrawGridUnits();
}

/* 自写：把一通道 samples 映射为曲线并限制在绘图区内。 */
static void App_DrawChannel(const float32_t *samples,
    uint16_t color)
{
    uint16_t point;
    for (point = 1U; point < APP_TRACE_POINTS; ++point) {
        uint16_t index0 = (uint16_t)(((uint32_t)(point - 1U) *
            (APP_SAMPLE_COUNT - 1U)) / (APP_TRACE_POINTS - 1U));
        uint16_t index1 = (uint16_t)(((uint32_t)point *
            (APP_SAMPLE_COUNT - 1U)) / (APP_TRACE_POINTS - 1U));
        (void)TFT_ST7789_DrawLine(&g_tft, App_MapX(point - 1U),
            App_MapY(samples[index0]), App_MapX(point),
            App_MapY(samples[index1]), color);
    }
}

/* 自写：查询网格背景颜色，擦线时恢复网格而不是涂成纯黑。 */
static uint16_t App_BackgroundColor(int32_t x, int32_t y)
{
    uint8_t division;
    int32_t grid_x;
    int32_t grid_y;
    const uint16_t grid_color = TFT_ST7789_RGB565(55U, 75U, 85U);
    if ((x == APP_PLOT_X + APP_PLOT_W / 2) ||
        (y == APP_PLOT_Y + APP_PLOT_H / 2)) {
        return TFT_ST7789_BLUE;
    }
    for (division = 1U; division < 5U; ++division) {
        grid_x = APP_PLOT_X + 1 + (int32_t)(((uint32_t)division *
            (APP_PLOT_W - 3U)) / 5U);
        if (x == grid_x) return grid_color;
    }
    for (division = 1U; division < 4U; ++division) {
        grid_y = APP_PLOT_Y + 1 + (int32_t)(((uint32_t)division *
            (APP_PLOT_H - 3U)) / 4U);
        if (y == grid_y) return grid_color;
    }
    return TFT_ST7789_BLACK;
}

/* 自写：沿旧线逐点恢复背景/网格，解决曲线移动后的残影。 */
static void App_RestoreLine(int32_t x0, int32_t y0,
    int32_t x1, int32_t y1)
{
    int32_t dx = (x1 > x0) ? x1 - x0 : x0 - x1;
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = (y1 > y0) ? y1 - y0 : y0 - y1;
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t error = dx - dy;
    for (;;) {
        (void)TFT_ST7789_DrawPixel(&g_tft, x0, y0,
            App_BackgroundColor(x0, y0));
        if ((x0 == x1) && (y0 == y1)) break;
        if ((2 * error) > -dy) {
            error -= dy;
            x0 += sx;
        }
        if ((2 * error) < dx) {
            error += dx;
            y0 += sy;
        }
    }
}

/* 自写：擦除一个通道上一帧保存的像素坐标。 */
static void App_ErasePreviousChannel(const int32_t *previous_y)
{
    uint16_t point;
    for (point = 1U; point < APP_TRACE_POINTS; ++point) {
        App_RestoreLine(App_MapX(point - 1U), previous_y[point - 1U],
            App_MapX(point), previous_y[point]);
    }
}

/* 自写组合：先擦除旧的 A/B 曲线，再按当前量程画新曲线并保存坐标。 */
static void App_DrawWaveforms(void)
{
    uint16_t point;
    if (g_previous_a_valid != 0U) {
        App_ErasePreviousChannel(g_previous_y_a);
    }
    if (g_previous_b_valid != 0U) {
        App_ErasePreviousChannel(g_previous_y_b);
    }
    g_previous_a_valid = 0U;
    g_previous_b_valid = 0U;
    if ((g_display_mode == 1U) || (g_display_mode == 3U)) {
        for (point = 0U; point < APP_TRACE_POINTS; ++point) {
            uint16_t index = (uint16_t)(((uint32_t)point *
                (APP_SAMPLE_COUNT - 1U)) / (APP_TRACE_POINTS - 1U));
            g_previous_y_a[point] = App_MapY(g_display_a[index]);
        }
        App_DrawChannel(g_display_a, TFT_ST7789_YELLOW);
        g_previous_a_valid = 1U;
    }
    if ((g_display_mode == 2U) || (g_display_mode == 3U)) {
        for (point = 0U; point < APP_TRACE_POINTS; ++point) {
            uint16_t index = (uint16_t)(((uint32_t)point *
                (APP_SAMPLE_COUNT - 1U)) / (APP_TRACE_POINTS - 1U));
            g_previous_y_b[point] = App_MapY(g_display_b[index]);
        }
        App_DrawChannel(g_display_b, TFT_ST7789_CYAN);
        g_previous_b_valid = 1U;
    }
}

/* 自写：把中断扫描到的稳定字符写入环形队列。 */
static void App_QueueKey(char symbol)
{
    uint8_t next = (uint8_t)((g_key_head + 1U) % APP_KEY_QUEUE_SIZE);
    if (next != g_key_tail) {
        g_key_queue[g_key_head] = symbol;
        g_key_head = next;
    }
}

/* 自写状态机：A/B/C/D 改通道或坐标档位；按键后置 display_due 立即刷新。 */
static void App_HandleKey(char symbol)
{
    uint32_t requested_rate;
    uint8_t target_x_range;
    if (symbol == 'A') {
        if (g_y_range_index > 0U) --g_y_range_index;
        App_DrawStatus();
        App_DrawGridUnits();
    } else if (symbol == 'B') {
        if (g_y_range_index + 1U < APP_Y_RANGE_COUNT) ++g_y_range_index;
        App_DrawStatus();
        App_DrawGridUnits();
    } else if ((symbol == 'C') || (symbol == 'D')) {
        target_x_range = g_x_range_index;
        if (symbol == 'C') {
            if (target_x_range > 0U) --target_x_range;
        } else if (target_x_range + 1U < APP_X_RANGE_COUNT) {
            ++target_x_range;
        }
        requested_rate = (APP_SAMPLE_COUNT * 1000000U) /
            g_x_span_us[target_x_range];
        /* 【双 ADC 模块】X 轴档位变化时同步修改硬件采样率，并读取实际配置值。 */
        if (SignalDualADC_SetSampleRate(requested_rate) == SIGNAL_RESULT_OK) {
            g_x_range_index = target_x_range;
            App_DrawStatus();
            App_DrawGridUnits();
        }
    } else if ((symbol >= '1') && (symbol <= '3')) {
        g_display_mode = (uint8_t)(symbol - '0');
        App_DrawStatus();
    }
}

/* 自写：主循环取出所有待处理按键，避免在 SysTick 中做绘图。 */
static void App_ProcessQueuedKeys(void)
{
    char symbol;
    while (g_key_tail != g_key_head) {
        symbol = g_key_queue[g_key_tail];
        g_key_tail = (uint8_t)((g_key_tail + 1U) % APP_KEY_QUEUE_SIZE);
        App_HandleKey(symbol);
    }
}

/* 自写中断：每 5 ms 调用键盘模块，500 ms 节拍只置显示标志。 */
void SysTick_Handler(void)
{
    static uint8_t milliseconds;
    char symbol;
    ++milliseconds;
    if (g_display_elapsed_ms < APP_DISPLAY_HOLD_MS) {
        ++g_display_elapsed_ms;
    } else {
        g_display_due = 1U;
    }
    if (milliseconds < APP_KEYPAD_SCAN_MS) return;
    milliseconds = 0U;
    /* 【矩阵键盘模块】完成固定引脚扫描、消抖和鬼键判断，main 只把字符入队。 */
    if (SignalMatrixKeypad4x4_ReadNewSymbol(&symbol) == SIGNAL_RESULT_OK) {
        App_QueueKey(symbol);
    }
}

/* main：初始化双 ADC 和屏幕，持续采样两路信号；每隔 APP_DISPLAY_HOLD_MS 才擦旧线
 * 并画新线，保持波形稳定。键盘改变 X/Y 量程或显示通道后置 display_due，立即刷新。 */
int main(void)
{
    const signal_dual_adc_config_t adc_config = {
        SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
    };

    SYSCFG_DL_init();
    /* 【双 ADC 模块】初始化同步触发和 DMA；adc_config.sample_rate_hz 是初始横轴采样率。 */
    if (SignalDualADC_Init(&adc_config) != SIGNAL_RESULT_OK) while (1) { }
    DL_DMA_enableInterrupt(DMA,
        DL_DMA_INTERRUPT_CHANNEL0 | DL_DMA_INTERRUPT_CHANNEL1);
    if (SignalTFTST7789_MSPM0_Init(&g_tft, TFT_ST7789_ROTATION_270,
            0U, 0U) != TFT_ST7789_OK) while (1) { }
    App_DrawStaticUi();
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (1) { }

    while (1) {
        App_ProcessQueuedKeys();
        /* 【双 ADC 模块】采集 A/B 两通道同一时刻的 APP_SAMPLE_COUNT 个样本。 */
        if (SignalDualADC_Start(g_raw_a, g_raw_b,
                APP_SAMPLE_COUNT) == SIGNAL_RESULT_OK) {
            while (!SignalDualADC_IsFinished()) {
                __WFI();
            }
            {
                uint16_t index;
                float32_t removed_dc_a;
                float32_t removed_dc_b;
                for (index = 0U; index < APP_SAMPLE_COUNT; ++index) {
                    g_display_a[index] = (float32_t)g_raw_a[index] * 3.3F /
                        4095.0F;
                    g_display_b[index] = (float32_t)g_raw_b[index] * 3.3F /
                        4095.0F;
                }
                /* Remove DC recipe: mean followed by an in-place negative offset. */
                /* 【CMSIS-DSP】mean 求 DC，offset 将整帧减去均值；不是 main 自写去直流公式。 */
                arm_mean_f32(g_display_a, APP_SAMPLE_COUNT, &removed_dc_a);
                arm_offset_f32(g_display_a, -removed_dc_a,
                    g_display_a, APP_SAMPLE_COUNT);
                arm_mean_f32(g_display_b, APP_SAMPLE_COUNT, &removed_dc_b);
                arm_offset_f32(g_display_b, -removed_dc_b,
                    g_display_b, APP_SAMPLE_COUNT);
            }
            if (g_display_due != 0U) {
                g_display_due = 0U;
                g_display_elapsed_ms = 0U;
                App_DrawWaveforms();
            }
        }
    }
}
