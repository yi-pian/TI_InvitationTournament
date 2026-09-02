/*
 * example08：多模式片上模拟前端双通道信号分析仪。
 *
 * 比赛大步骤：
 * 1) GPAMP 缓冲 PA26；2) OPA0 片上单位增益缓冲；3) OPA1 片内 DAC8 基准缓冲；
 * 4) COMP0 硬件过零/门限；5) 双 ADC+DMA；6) ST7789+键盘；7) 相位计算。
 *
 * modules/ 的 API 调用均从各 README 复制；本文件的 App_* 仅是题目组合逻辑。
 * ADC_A=OPA0 的内部缓冲输出，ADC_B=GPAMP 的内部缓冲输出。
 */
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_status.h"
#include "signal_algorithm_status.h"
#include "signal_opa.h"
#include "signal_opa_to_adc.h"
#include "signal_gpamp.h"
#include "signal_gpamp_buffer.h"
#include "signal_comparator.h"
#include "signal_comparator_threshold.h"
#include "signal_comparator_zero_cross.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_dual_adc_phase.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"

#define APP_KEYPAD_SCAN_PERIOD_MS (5U)
#define APP_PLOT_X                (4)
#define APP_PLOT_Y                (28)
#define APP_PLOT_SIZE             (190)
#define APP_PLOT_INNER_X          (APP_PLOT_X + 1)
#define APP_PLOT_INNER_Y          (APP_PLOT_Y + 1)
#define APP_PLOT_INNER_SIZE       (APP_PLOT_SIZE - 2)
#define APP_PLOT_TRACE_MARGIN     (4)
#define APP_PLOT_POINTS           (SIGNAL_SAMPLE_COUNT)
#define APP_MIN_DISPLAY_RANGE     (64U)
#define APP_ZERO_CROSS_DAC_CODE   (128U) /* 1.65 V，正常相位测量用 */
#define APP_THRESHOLD_DAC_CODE    (155U) /* 约 2.00 V，门限实验用 */
#define APP_COMP_EDGE_INTERRUPTS  (DL_COMP_INTERRUPT_OUTPUT_EDGE | \
                                  DL_COMP_INTERRUPT_OUTPUT_EDGE_INV)
#define APP_COMP_DAC_TO_ADC12(code) ((uint16_t)((uint16_t)(code) << 4U))

/* 步骤 5：两组 DMA 目的缓冲；每次采样从两颗 ADC 同一计时器事件开始。 */
static uint16_t g_raw_a[SIGNAL_SAMPLE_COUNT];
static uint16_t g_raw_b[SIGNAL_SAMPLE_COUNT];

static tft_st7789_t g_tft;
static volatile signal_result_t g_adc_status;
static volatile tft_st7789_status_t g_tft_status;
static volatile signal_result_t g_key_status;

/* 步骤 4：COMP0 ISR 只修改这两个计数器，主循环仅显示。 */
static volatile uint32_t g_comp_rise_count;
static volatile uint32_t g_comp_fall_count;
static volatile uint8_t g_comp_threshold_mode;
/* PA26 外接输入时，必须先接入确定电平再由 B/C 打开捕获，防止悬空脚触发中断风暴。 */
static volatile uint8_t g_comp_capture_enabled;

/* 步骤 6：与 22_X 相同，SysTick 直接分发已消抖的新按键，只改轻量状态。 */
static volatile uint8_t g_frequency_ratio = 1U;
static volatile uint8_t g_page;
static volatile uint8_t g_ratio_display_revision = 1U;
static uint8_t g_ratio_displayed_revision;
static volatile uint8_t g_comp_display_revision = 1U;
static uint8_t g_comp_displayed_revision;
static volatile uint8_t g_page_display_revision = 1U;
static uint8_t g_page_displayed_revision;

/* 步骤 7：相位模块的输出和输入参数。 */
static int16_t g_phase_degrees;
static uint8_t g_phase_valid;
static signal_dual_adc_phase_config_t g_phase_config = {
    .hysteresis_code = 16U, .min_amplitude_code = 64U,
    .frequency_ratio = 1U, .max_x_crossings = 16U, .max_y_crossings = 64U
};

/* 步骤 1~4：README 的软件预算配置；真正的模拟开关连接由 SysConfig 完成。 */
static signal_gpamp_config_t g_gpamp_config;
static signal_opa_config_t g_pga_config = {
    .mode = SIGNAL_OPA_MODE_BUFFER
};
static signal_opa_config_t g_dac_buffer_config = {
    .mode = SIGNAL_OPA_MODE_BUFFER
};
static signal_comparator_config_t g_zero_cross_config;
static signal_comparator_config_t g_threshold_config;
static float g_pga_gain;
static float g_dac_buffer_gain;
static float g_pga_low_margin_v;
static float g_pga_high_margin_v;
static float g_dac_buffer_low_margin_v;
static float g_dac_buffer_high_margin_v;
static signal_result_t g_frontend_status;
/* 每帧由主循环更新；既供自动量程绘图，也直接显示，便于现场排查接线。 */
static uint16_t g_raw_a_min;
static uint16_t g_raw_a_max;
static uint16_t g_raw_b_min;
static uint16_t g_raw_b_max;

/* 自写：寻找本帧最小/最大 ADC 码。自动量程只影响显示，不改变相位算法原始数据。 */
static void App_FindBounds(const uint16_t *samples, uint16_t *minimum,
    uint16_t *maximum)
{
    uint16_t index;
    *minimum = samples[0];
    *maximum = samples[0];
    for (index = 1U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        if (samples[index] < *minimum) *minimum = samples[index];
        if (samples[index] > *maximum) *maximum = samples[index];
    }
}

/* 自写：按本帧范围映射横轴。轨迹四周保留 4 像素，不让自动量程贴住蓝框。 */
static int32_t App_MapX(uint16_t sample, uint16_t minimum, uint16_t maximum)
{
    uint16_t range = (uint16_t)(maximum - minimum);
    if (range < APP_MIN_DISPLAY_RANGE)
        return APP_PLOT_INNER_X + APP_PLOT_INNER_SIZE / 2;
    return APP_PLOT_INNER_X + APP_PLOT_TRACE_MARGIN + (int32_t)
        (((uint32_t)(sample - minimum) * (APP_PLOT_INNER_SIZE - 1U -
        2U * APP_PLOT_TRACE_MARGIN)) / range);
}

/* 自写：自动量程纵轴并翻转屏幕 Y，使较高电压位于上方且留边。 */
static int32_t App_MapY(uint16_t sample, uint16_t minimum, uint16_t maximum)
{
    uint16_t range = (uint16_t)(maximum - minimum);
    if (range < APP_MIN_DISPLAY_RANGE)
        return APP_PLOT_INNER_Y + APP_PLOT_INNER_SIZE / 2;
    return APP_PLOT_INNER_Y + APP_PLOT_INNER_SIZE - 1 - APP_PLOT_TRACE_MARGIN -
        (int32_t)(((uint32_t)(sample - minimum) * (APP_PLOT_INNER_SIZE - 1U -
        2U * APP_PLOT_TRACE_MARGIN)) / range);
}

/* 自写：固定边框和坐标轴全部用 FillRect，彻底避开有阻塞风险的 DrawLine/DrawRect。 */
static tft_st7789_status_t App_DrawPlotFrame(void)
{
    tft_st7789_status_t status;
    status = TFT_ST7789_FillRect(&g_tft, APP_PLOT_X, APP_PLOT_Y,
        APP_PLOT_SIZE, APP_PLOT_SIZE, TFT_ST7789_BLACK);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_FillRect(&g_tft, APP_PLOT_X, APP_PLOT_Y,
        APP_PLOT_SIZE, 1, TFT_ST7789_BLUE);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_FillRect(&g_tft, APP_PLOT_X,
        APP_PLOT_Y + APP_PLOT_SIZE - 1, APP_PLOT_SIZE, 1, TFT_ST7789_BLUE);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_FillRect(&g_tft, APP_PLOT_X, APP_PLOT_Y,
        1, APP_PLOT_SIZE, TFT_ST7789_BLUE);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_FillRect(&g_tft, APP_PLOT_X + APP_PLOT_SIZE - 1,
        APP_PLOT_Y, 1, APP_PLOT_SIZE, TFT_ST7789_BLUE);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_FillRect(&g_tft, APP_PLOT_X + APP_PLOT_SIZE / 2,
        APP_PLOT_Y + 1, 1, APP_PLOT_SIZE - 2, TFT_ST7789_BLUE);
    if (status != TFT_ST7789_OK) return status;
    return TFT_ST7789_FillRect(&g_tft, APP_PLOT_X + 1,
        APP_PLOT_Y + APP_PLOT_SIZE / 2, APP_PLOT_SIZE - 2, 1, TFT_ST7789_BLUE);
}

/* 自写：只把端点均在绘图区的线段交给 README 的 DrawLine。
 * 矩形是凸集，两个端点都在其中时整段必然在其中；端点异常才拒绝。 */
static tft_st7789_status_t App_DrawSafeLine(int32_t x0, int32_t y0,
    int32_t x1, int32_t y1)
{
    int32_t delta_x = x1 - x0;
    int32_t delta_y = y1 - y0;
    int32_t abs_x = (delta_x < 0) ? -delta_x : delta_x;
    int32_t abs_y = (delta_y < 0) ? -delta_y : delta_y;

    if ((x0 < APP_PLOT_INNER_X + APP_PLOT_TRACE_MARGIN) ||
        (x0 > APP_PLOT_INNER_X + APP_PLOT_INNER_SIZE - 1 - APP_PLOT_TRACE_MARGIN) ||
        (x1 < APP_PLOT_INNER_X + APP_PLOT_TRACE_MARGIN) ||
        (x1 > APP_PLOT_INNER_X + APP_PLOT_INNER_SIZE - 1 - APP_PLOT_TRACE_MARGIN) ||
        (y0 < APP_PLOT_INNER_Y + APP_PLOT_TRACE_MARGIN) ||
        (y0 > APP_PLOT_INNER_Y + APP_PLOT_INNER_SIZE - 1 - APP_PLOT_TRACE_MARGIN) ||
        (y1 < APP_PLOT_INNER_Y + APP_PLOT_TRACE_MARGIN) ||
        (y1 > APP_PLOT_INNER_Y + APP_PLOT_INNER_SIZE - 1 - APP_PLOT_TRACE_MARGIN) ||
        ((abs_x == 0) && (abs_y == 0))) return TFT_ST7789_OK;
    return TFT_ST7789_DrawLine(&g_tft, x0, y0, x1, y1, TFT_ST7789_YELLOW);
}

/* 自写：显示完整 512 点；相位算法使用同一帧，避免低频只显示帧开头的一小段。
 * 每一对连续采样点均由已修复的 DrawLine 连成实线。 */
static tft_st7789_status_t App_DrawLissajous(void)
{
    uint16_t point;
    tft_st7789_status_t status = TFT_ST7789_FillRect(&g_tft,
        APP_PLOT_INNER_X, APP_PLOT_INNER_Y, APP_PLOT_INNER_SIZE,
        APP_PLOT_INNER_SIZE, TFT_ST7789_BLACK);
    if (status != TFT_ST7789_OK) return status;
    for (point = 0U; point < APP_PLOT_POINTS; ++point) {
        uint32_t index = point;
        int32_t x = App_MapX(g_raw_a[index], g_raw_a_min, g_raw_a_max);
        int32_t y = App_MapY(g_raw_b[index], g_raw_b_min, g_raw_b_max);

        /* 每个有效抽样点都可见；当 ADC 无有效摆幅时，它们会重合在图心。 */
        status = TFT_ST7789_DrawPixel(&g_tft,
            x, y, TFT_ST7789_YELLOW);
        if (status != TFT_ST7789_OK) return status;

        if ((point != 0U) &&
            ((g_raw_a_max - g_raw_a_min) >= APP_MIN_DISPLAY_RANGE) &&
            ((g_raw_b_max - g_raw_b_min) >= APP_MIN_DISPLAY_RANGE)) {
            uint32_t previous = point - 1U;
            int32_t previous_x = App_MapX(g_raw_a[previous], g_raw_a_min,
                g_raw_a_max);
            int32_t previous_y = App_MapY(g_raw_b[previous], g_raw_b_min,
                g_raw_b_max);
            /* 每段先验证端点，再调用已修复的 DrawLine；不再人为丢弃长段。 */
            status = App_DrawSafeLine(previous_x, previous_y, x, y);
            if (status != TFT_ST7789_OK) return status;
        }
    }
    return TFT_ST7789_OK;
}

/* 22_X 同款局部刷新：先擦掉一个数字/文字字段，不重画标签、边框和其余屏幕。 */
static tft_st7789_status_t App_ClearField(int32_t x, int32_t y, int32_t width)
{
    return TFT_ST7789_FillRect(&g_tft, x, y, width, 16,
        TFT_ST7789_BLACK);
}

static tft_st7789_status_t App_DrawRatio(void)
{
    tft_st7789_status_t status = App_ClearField(264, 32, 16);
    if (status != TFT_ST7789_OK) return status;
    return TFT_ST7789_DrawInt32(&g_tft, 264, 32, g_frequency_ratio,
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
}

static tft_st7789_status_t App_DrawPhase(void)
{
    tft_st7789_status_t status = App_ClearField(256, 56, 56);
    if (status != TFT_ST7789_OK) return status;
    if (g_phase_valid == 0U) return TFT_ST7789_DrawString(&g_tft, 256, 56,
        "--", TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK,
        false, false);
    return TFT_ST7789_DrawInt32(&g_tft, 256, 56, g_phase_degrees,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
}

static tft_st7789_status_t App_DrawComparatorMode(void)
{
    tft_st7789_status_t status = App_ClearField(256, 82, 64);
    if (status != TFT_ST7789_OK) return status;
    return TFT_ST7789_DrawString(&g_tft, 256, 82,
        (g_comp_threshold_mode == 0U) ? "ZERO" : "THR",
        TFT_ST7789_FONT_8X16, TFT_ST7789_MAGENTA, TFT_ST7789_BLACK, false, false);
}

static tft_st7789_status_t App_DrawCounter(int32_t y, uint32_t value)
{
    tft_st7789_status_t status = App_ClearField(248, y, 72);
    if (status != TFT_ST7789_OK) return status;
    return TFT_ST7789_DrawInt32(&g_tft, 248, y, (int32_t)value,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
}

/* 现场诊断：只更新右侧的两组 RAW 最小/最大 ADC 码，不打断波形局部刷新。 */
static tft_st7789_status_t App_DrawRawRange(int32_t y, uint16_t minimum,
    uint16_t maximum)
{
    tft_st7789_status_t status = App_ClearField(248, y, 72);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawInt32(&g_tft, 248, y, minimum,
        TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 280, y, "-",
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    return TFT_ST7789_DrawInt32(&g_tft, 288, y, maximum,
        TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
}

/* 页面切换时才调用：允许清一次页面，正常采样循环绝不全页刷新。 */
static tft_st7789_status_t App_DrawDiagnosticStatic(void)
{
    tft_st7789_status_t status = TFT_ST7789_FillRect(&g_tft, 4, 28, 316, 190,
        TFT_ST7789_BLACK);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 8, 36, "PA26 -> GPAMP -> ADC1",
        TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 8, 62, "OPA0 BUF -> ADC0", TFT_ST7789_FONT_8X16,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawFloat(&g_tft, 128, 62, g_pga_gain, 1,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 8, 88, "OPA1 DAC BUF", TFT_ST7789_FONT_8X16,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawFloat(&g_tft, 128, 88, g_dac_buffer_gain, 1,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 8, 114, "ADC0/ADC1", TFT_ST7789_FONT_8X16,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 128, 114, "CH13/CH14",
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 8, 140, "COMP DAC", TFT_ST7789_FONT_8X16,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    return TFT_ST7789_DrawInt32(&g_tft, 128, 140,
        (g_comp_threshold_mode == 0U) ? APP_ZERO_CROSS_DAC_CODE : APP_THRESHOLD_DAC_CODE,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
}

/* 正常循环只刷新诊断页中会变化的 COMP DAC 数字。 */
static tft_st7789_status_t App_DrawDiagnosticDac(void)
{
    tft_st7789_status_t status = App_ClearField(128, 140, 48);
    if (status != TFT_ST7789_OK) return status;
    return TFT_ST7789_DrawInt32(&g_tft, 128, 140,
        (g_comp_threshold_mode == 0U) ? APP_ZERO_CROSS_DAC_CODE : APP_THRESHOLD_DAC_CODE,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
}

/* 22_X 同款静态框架：标题、标签、边框只在上电或换页时画一次。 */
static tft_st7789_status_t App_DrawMainStatic(void)
{
    tft_st7789_status_t status = TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 4, 4, "ANALOG FRONTEND",
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    status = App_DrawPlotFrame();
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 200, 32, "RATIO", TFT_ST7789_FONT_8X16,
        TFT_ST7789_CYAN, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 200, 56, "PHASE", TFT_ST7789_FONT_8X16,
        TFT_ST7789_CYAN, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 200, 82, "COMP", TFT_ST7789_FONT_8X16,
        TFT_ST7789_CYAN, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 200, 108, "RISE", TFT_ST7789_FONT_8X16,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 200, 132, "FALL", TFT_ST7789_FONT_8X16,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    status = TFT_ST7789_DrawString(&g_tft, 200, 156, "A RAW", TFT_ST7789_FONT_8X16,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK, false, false);
    if (status != TFT_ST7789_OK) return status;
    return TFT_ST7789_DrawString(&g_tft, 200, 180, "B RAW", TFT_ST7789_FONT_8X16,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK, false, false);
}

static tft_st7789_status_t App_DrawPageStatic(uint8_t page)
{
    return (page == 0U) ? App_DrawMainStatic() : App_DrawDiagnosticStatic();
}

/* README 补充的硬件桥接：切换 COMP DAC8，同时它也成为 OPA0 的虚地。 */
static void App_SetComparatorMode(uint8_t threshold_mode)
{
    g_comp_threshold_mode = threshold_mode;
    g_comp_rise_count = 0U;
    g_comp_fall_count = 0U;
    DL_COMP_setDACCode0(SIGNAL_COMP_ZERO_CROSS_INST,
        (threshold_mode == 0U) ? APP_ZERO_CROSS_DAC_CODE : APP_THRESHOLD_DAC_CODE);
    ++g_comp_display_revision;
}

/* 自写的现场保护：已确认本板 COMP0 异步边沿 IRQ 会使屏幕卡住，故 NVIC 永久关闭。
 * B/C 仅开启本帧 ADC 边沿统计；比较门限仍与已配置的 COMP0.DAC8 完全相同。 */
static void App_StartComparatorCount(void)
{
    NVIC_DisableIRQ(SIGNAL_COMP_ZERO_CROSS_INST_INT_IRQN);
    g_comp_capture_enabled = 1U;
}

/* 自写的稳定计数：用 OPA0 缓冲 ADC 帧按 COMP0 同一 DAC 门限统计边沿。
 * 这样仍可验证 1.65 V/2.00 V 门限功能，且不会进入已证实不稳定的 COMP0 IRQ。 */
static void App_CountComparatorEdgesFromADC(void)
{
    uint16_t index;
    uint16_t threshold = APP_COMP_DAC_TO_ADC12((g_comp_threshold_mode == 0U) ?
        APP_ZERO_CROSS_DAC_CODE : APP_THRESHOLD_DAC_CODE);

    if (g_comp_capture_enabled == 0U) return;
    for (index = 1U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        if ((g_raw_a[index - 1U] < threshold) && (g_raw_a[index] >= threshold))
            ++g_comp_rise_count;
        else if ((g_raw_a[index - 1U] >= threshold) && (g_raw_a[index] < threshold))
            ++g_comp_fall_count;
    }
}

/* 复制 TI COMP 示例的中断分发；每个硬件边沿只计一次，避免 ISR 做重任务。 */
void COMP0_IRQHandler(void)
{
    switch (DL_COMP_getPendingInterrupt(SIGNAL_COMP_ZERO_CROSS_INST)) {
        case DL_COMP_IIDX_OUTPUT_EDGE: ++g_comp_rise_count; break;
        case DL_COMP_IIDX_OUTPUT_EDGE_INV: ++g_comp_fall_count; break;
        default: break;
    }
}

static void App_ProcessKey(char key);

/* 与 22_X 相同：每 5 ms 读取“新稳定按下”符号，并在 ISR 直接分发轻量状态。 */
void SysTick_Handler(void)
{
    static uint8_t milliseconds;
    char symbol;
    ++milliseconds;
    if (milliseconds < APP_KEYPAD_SCAN_PERIOD_MS) return;
    milliseconds = 0U;
    g_key_status = SignalMatrixKeypad4x4_ReadNewSymbol(&symbol);
    if (g_key_status == SIGNAL_RESULT_OK) App_ProcessKey(symbol);
}

/* 与 22_X 的 App_Process*Key 相同：只改状态/版本号，不在中断中 SPI 绘图。 */
static void App_ProcessKey(char key)
{
    if ((key >= '1') && (key <= '5')) {
        g_frequency_ratio = (uint8_t)(key - '0');
        ++g_ratio_display_revision;
    } else if ((key == 'A') && (g_page != 0U)) {
        g_page = 0U;
        ++g_page_display_revision;
    } else if ((key == 'D') && (g_page == 0U)) {
        g_page = 1U;
        ++g_page_display_revision;
    }
    else if (key == 'B') {
        App_SetComparatorMode(0U);
        App_StartComparatorCount();
    } else if (key == 'C') {
        App_SetComparatorMode(1U);
        App_StartComparatorCount();
    }
}

int main(void)
{
    /* 步骤 5：复制双 ADC README 的定时器采样配置。 */
    const signal_dual_adc_config_t adc_config = {
        .sample_rate_hz = SIGNAL_SAMPLE_RATE_HZ,
        .timer_clock_hz = CPUCLK_FREQ, .timer_max_count = 65536U
    };
    /* 步骤 2/3：两颗 OPA 均按 README 的 BUFFER 配置做量程预算。 */
    /* 步骤 1/4：复制 GPAMP 缓冲、比较器过零和门限 README 的调用。 */
    signal_result_t gpamp_result = SignalGPAMPBuffer_MakeConfig(1.65f, &g_gpamp_config);
    signal_result_t zero_result = SignalComparatorZeroCross_MakeConfig(
        1.65f, 0.02f, &g_zero_cross_config);
    signal_result_t threshold_result = SignalComparatorThreshold_MakeConfig(
        2.00f, 0.02f, false, &g_threshold_config);
    /* 两路 OPA-to-ADC 量程预算：两路输出均不能越过 0~3.3 V。 */
    const signal_opa_to_adc_budget_t pga_budget = { 1.45f, 1.85f, 0.0f, SIGNAL_ADC_VREF_V };
    const signal_opa_to_adc_budget_t dac_buffer_budget = { 1.60f, 1.70f, 0.0f, SIGNAL_ADC_VREF_V };

    /* 所有片上模拟外设都由 SysConfig 先初始化，不能手改生成文件。 */
    SYSCFG_DL_init();
    g_frontend_status = SignalOPA_CalculateGain(&g_pga_config, &g_pga_gain);
    if (g_frontend_status == SIGNAL_RESULT_OK)
        g_frontend_status = SignalOPA_CalculateGain(&g_dac_buffer_config, &g_dac_buffer_gain);
    if (g_frontend_status == SIGNAL_RESULT_OK)
        g_frontend_status = SignalOPAToADC_CheckRange(&pga_budget,
            &g_pga_low_margin_v, &g_pga_high_margin_v);
    if (g_frontend_status == SIGNAL_RESULT_OK)
        g_frontend_status = SignalOPAToADC_CheckRange(&dac_buffer_budget,
            &g_dac_buffer_low_margin_v, &g_dac_buffer_high_margin_v);
    if ((gpamp_result != SIGNAL_RESULT_OK) || (zero_result != SIGNAL_RESULT_OK) ||
        (threshold_result != SIGNAL_RESULT_OK) || (g_frontend_status != SIGNAL_RESULT_OK)) while (1) { }

    /* 步骤 4：SysConfig 配置 COMP0 和 DAC8，但实板的异步边沿 IRQ 不稳定，NVIC 必须关闭。
     * B/C 后改用同门限的 ADC 帧统计来显示 RISE/FALL，避免卡死。 */
    NVIC_DisableIRQ(SIGNAL_COMP_ZERO_CROSS_INST_INT_IRQN);
    DL_COMP_clearInterruptStatus(SIGNAL_COMP_ZERO_CROSS_INST,
        APP_COMP_EDGE_INTERRUPTS);
    App_SetComparatorMode(0U);

    /* 步骤 5/6：复制模块 README 的 ADC 初始化、LCD 初始化、清屏和标题输出。 */
    g_adc_status = SignalDualADC_Init(&adc_config);
    if (g_adc_status != SIGNAL_RESULT_OK) while (1) { }
    g_tft_status = SignalTFTST7789_MSPM0_Init(&g_tft, TFT_ST7789_ROTATION_270, 0U, 0U);
    if (g_tft_status != TFT_ST7789_OK) while (1) { }
    /* 22_X 同款：上电时画一次静态页面，之后不再 FillScreen/重画标签。 */
    g_tft_status = App_DrawPageStatic(g_page);
    if (g_tft_status != TFT_ST7789_OK) while (1) { }
    g_tft_status = App_DrawRatio();
    if (g_tft_status == TFT_ST7789_OK) g_tft_status = App_DrawComparatorMode();
    if (g_tft_status == TFT_ST7789_OK) g_tft_status = App_DrawPhase();
    if (g_tft_status == TFT_ST7789_OK) g_tft_status = App_DrawCounter(108, g_comp_rise_count);
    if (g_tft_status == TFT_ST7789_OK) g_tft_status = App_DrawCounter(132, g_comp_fall_count);
    if (g_tft_status == TFT_ST7789_OK) g_tft_status = App_DrawRawRange(156, g_raw_a_min, g_raw_a_max);
    if (g_tft_status == TFT_ST7789_OK) g_tft_status = App_DrawRawRange(180, g_raw_b_min, g_raw_b_max);
    if (g_tft_status != TFT_ST7789_OK) while (1) { }
    g_ratio_displayed_revision = g_ratio_display_revision;
    g_comp_displayed_revision = g_comp_display_revision;
    g_page_displayed_revision = g_page_display_revision;
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (1) { }

    while (1) {
        signal_dual_adc_phase_result_t phase_result;
        signal_algorithm_status_t phase_status;

        /* 步骤 5：复制双 ADC README 的 Start/IsFinished/WFI 闭环。 */
        g_adc_status = SignalDualADC_Start(g_raw_a, g_raw_b, SIGNAL_SAMPLE_COUNT);
        if (g_adc_status != SIGNAL_RESULT_OK) while (1) { }
        while (!SignalDualADC_IsFinished()) { __WFI(); }

        /* 自写的显示辅助：先取本帧范围，供自动量程轨迹和 RAW 接线诊断共同使用。 */
        App_FindBounds(g_raw_a, &g_raw_a_min, &g_raw_a_max);
        App_FindBounds(g_raw_b, &g_raw_b_min, &g_raw_b_max);

        /* 步骤 7：复制相位模块 README 的 Process 调用，保存有效性和角度。 */
        g_phase_config.frequency_ratio = g_frequency_ratio;
        phase_status = SignalDualADCPhase_Process(g_raw_a, g_raw_b,
            SIGNAL_SAMPLE_COUNT, SIGNAL_SAMPLE_RATE_HZ, &g_phase_config, &phase_result);
        if ((phase_status == SIGNAL_ALGORITHM_OK) && (phase_result.valid != 0U)) {
            g_phase_degrees = phase_result.phase_degrees;
            g_phase_valid = 1U;
        } else g_phase_valid = 0U;

        /* 仅在用户 B/C 开始门限实验后累加本帧的上升/下降穿越次数。 */
        App_CountComparatorEdgesFromADC();

        /* 22_X 同款 revision 调度：页面静态内容仅在 A/D 切换时重画一次。 */
        if (g_page_displayed_revision != g_page_display_revision) {
            g_tft_status = App_DrawPageStatic(g_page);
            if (g_tft_status != TFT_ST7789_OK) while (1) { }
            g_page_displayed_revision = g_page_display_revision;
            /* 换回主页面后强制补画局部数字，不需要全屏重画。 */
            g_ratio_displayed_revision = 0U;
            g_comp_displayed_revision = 0U;
        }

        if (g_page == 0U) {
            /* 每帧只擦除绘图区内部，再画最新波形；蓝色边框/轴不刷新。 */
            g_tft_status = App_DrawLissajous();
            if (g_tft_status == TFT_ST7789_OK &&
                g_ratio_displayed_revision != g_ratio_display_revision) {
                g_tft_status = App_DrawRatio();
                g_ratio_displayed_revision = g_ratio_display_revision;
            }
            if (g_tft_status == TFT_ST7789_OK &&
                g_comp_displayed_revision != g_comp_display_revision) {
                g_tft_status = App_DrawComparatorMode();
                g_comp_displayed_revision = g_comp_display_revision;
            }
            /* 相位和边沿计数是采样结果，故每轮只刷新它们各自的数字矩形。 */
            if (g_tft_status == TFT_ST7789_OK) g_tft_status = App_DrawPhase();
            if (g_tft_status == TFT_ST7789_OK)
                g_tft_status = App_DrawCounter(108, g_comp_rise_count);
            if (g_tft_status == TFT_ST7789_OK)
                g_tft_status = App_DrawCounter(132, g_comp_fall_count);
            if (g_tft_status == TFT_ST7789_OK)
                g_tft_status = App_DrawRawRange(156, g_raw_a_min, g_raw_a_max);
            if (g_tft_status == TFT_ST7789_OK)
                g_tft_status = App_DrawRawRange(180, g_raw_b_min, g_raw_b_max);
        } else if (g_comp_displayed_revision != g_comp_display_revision) {
            /* 诊断页不重画固定文本和增益，只更新发生变化的 DAC 数字。 */
            g_tft_status = App_DrawDiagnosticDac();
            g_comp_displayed_revision = g_comp_display_revision;
        }
        if (g_tft_status != TFT_ST7789_OK) while (1) { }
    }
}
