/* ============================================================
 * 工程：06_trigger_burst_capture
 * 用途：软件边沿触发的 SINGLE/猝发捕获仪。
 * 输入：ADC CH1=PA25；输出：ST7789；控制：4x4 键盘。
 * KEY MAP：D=ARM/Re-arm，C=上升/下降沿，*=降低触发电平，#=提高；
 * 1/2/3=预触发 25%/50%/75%。
 * 来源：23_trigger_capture、21_time_domain_waveform、70、80；
 * 平台闭包来自 example04。经授权复制 modules/.syscfg，模块未修改。
 * ============================================================ */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_trigger_capture.h"

#define SEARCH_COUNT             (512U)
#define CAPTURE_COUNT            (256U)
#define SAMPLE_RATE_REQUEST_HZ   (200000U)
#define ADC_REFERENCE_V          (3.3f)
#define GRAPH_X                  (8)
#define GRAPH_Y                  (76)
#define GRAPH_W                  (304)
#define GRAPH_H                  (132)
#define KEYPAD_SCAN_MS           (5U)
#define KEY_QUEUE_SIZE           (8U)

typedef enum { CAPTURE_ARM = 0U, CAPTURE_WAITING,
    CAPTURE_TRIGGERED, CAPTURE_HOLD } capture_state_t;
typedef enum { PAGE_CAPTURE = 0U } app_page_t;

static uint16_t adc_samples[SEARCH_COUNT];
static uint16_t adc_unused_samples[SEARCH_COUNT];
static uint16_t captured_samples[CAPTURE_COUNT];
static float captured_voltage[CAPTURE_COUNT];
static float sample_rate_hz = (float)SAMPLE_RATE_REQUEST_HZ;
static uint16_t trigger_level = 2048U;
static uint16_t trigger_hysteresis = 24U;
static signal_trigger_edge_t trigger_slope = SIGNAL_TRIGGER_RISING;
static uint8_t pretrigger_percent = 25U;
static uint32_t trigger_index;
static float capture_vpp_v;
static capture_state_t capture_state = CAPTURE_ARM;
static app_page_t current_page = PAGE_CAPTURE;
static tft_st7789_t tft;
static volatile char key_queue[KEY_QUEUE_SIZE];
static volatile uint8_t key_queue_head;
static volatile uint8_t key_queue_tail;
static bool display_dirty = true;
static bool static_ui_drawn;

static void DrawText(int32_t x, int32_t y, const char *text, uint16_t color)
{
    (void)TFT_ST7789_DrawString(&tft, x, y, text, TFT_ST7789_FONT_8X16,
        color, TFT_ST7789_BLACK, false, false);
}

/* ============================================================
 * [函数] AcquireADCFrame
 * [功能] 取得一帧 CH1 搜索数据；CH2 仅作为同步 DMA 占位。
 * [来源] [FUYONG_ADAPTED] 04_dual_adc_dma/AcquireDualADCFrame。
 * [输入] 无；[输出] adc_samples[]；[单位] ADC code。
 * [全局] 两路 DMA 数组/sample_rate_hz。
 * [步骤] Start→等待 finished→读取实际 Fs。
 * [原因] WAITING 时每帧只采一次，触发算法直接复用该帧。
 * [单帧唯一] 是；[复用] Trigger_Capture。
 * [差异] 只分析 CH1；同步采集算法不变。
 * [依赖] signal_dual_adc_mspm0g3507。
 * ============================================================ */
static bool AcquireADCFrame(void)
{
    if (SignalDualADC_Start(adc_samples, adc_unused_samples,
            SEARCH_COUNT) != SIGNAL_RESULT_OK) return false;
    while (!SignalDualADC_IsFinished()) __WFI();
    sample_rate_hz = (float)SignalDualADC_GetConfiguredRate();
    return sample_rate_hz > 0.0f;
}

/* ============================================================
 * [函数] Trigger_Capture
 * [功能] 在搜索帧中查找带迟滞边沿，并提取预/后触发窗口。
 * [来源] [FUYONG_ADAPTED] 23_trigger_capture/TRIGGER_CAPTURE。
 * [输入] adc_samples、level/hysteresis/slope/pretrigger；
 * [输出] captured_samples、trigger_index。
 * [单位] ADC code/sample；[全局] 触发配置与捕获数组。
 * [步骤] SignalTrigger_Find→按百分数算 pretrigger→Extract。
 * [原因] 复用已验证触发模块，不在应用层重写迟滞状态机。
 * [单帧唯一] 每个 WAITING 帧一次；[复用] HOLD 波形和统计。
 * [差异] 增加可调斜率和预触发比例。
 * [依赖] signal_trigger_capture。
 * ============================================================ */
static bool Trigger_Capture(void)
{
    const signal_trigger_config_t config = {
        trigger_level, trigger_hysteresis, trigger_slope
    };
    size_t found_index;
    const size_t pretrigger_count =
        (size_t)CAPTURE_COUNT * pretrigger_percent / 100U;
    if (SignalTrigger_Find(adc_samples, SEARCH_COUNT, &config,
            pretrigger_count, &found_index) != SIGNAL_RESULT_OK) return false;
    if (SignalTrigger_Extract(adc_samples, SEARCH_COUNT, found_index,
            pretrigger_count, captured_samples,
            CAPTURE_COUNT) != SIGNAL_RESULT_OK) return false;
    trigger_index = (uint32_t)found_index;
    return true;
}

/* ============================================================
 * [函数] PrepareCaptureResult
 * [功能] 捕获成功后仅一次完成 code→V 和 Vpp 统计。
 * [来源] [FUYONG_ADAPTED] 30_basic_measurement/ConvertADCToVoltage。
 * [输入] captured_samples；[输出] captured_voltage/capture_vpp_v。
 * [单位] code→V；[全局] 捕获数组。
 * [步骤] 逐点换算并同遍更新 min/max。
 * [原因] 显示与数值共用同一转换结果，避免重复转换。
 * [单帧唯一] 是；[复用] DrawCaptureWaveform 和标题数值。
 * [差异] 合并 min/max；公式不变；[依赖] 无。
 * ============================================================ */
static void PrepareCaptureResult(void)
{
    uint32_t index;
    float minimum_v, maximum_v;
    for (index = 0U; index < CAPTURE_COUNT; ++index) {
        captured_voltage[index] = (float)captured_samples[index] *
            ADC_REFERENCE_V / 4095.0f;
        if (index == 0U) {
            minimum_v = captured_voltage[index]; maximum_v = minimum_v;
        } else {
            if (captured_voltage[index] < minimum_v) minimum_v = captured_voltage[index];
            if (captured_voltage[index] > maximum_v) maximum_v = captured_voltage[index];
        }
    }
    capture_vpp_v = maximum_v - minimum_v;
}

/* [READY_PROJECT_LOCAL]
 * ARM 只清除旧捕获语义并进入 WAITING；真正采样仍由主循环执行，
 * 从而不会在键盘 ISR 中启动 DMA。 */
static void ArmCapture(void)
{
    capture_state = CAPTURE_ARM;
    display_dirty = true;
}

/* [READY_PROJECT_LOCAL] 简单按键状态机；所有参数均限幅到 ADC code 范围。 */
static void HandleKeypad(void)
{
    char key;
    while (key_queue_tail != key_queue_head) {
        key = key_queue[key_queue_tail];
        key_queue_tail = (uint8_t)((key_queue_tail + 1U) % KEY_QUEUE_SIZE);
        if (key == 'D') ArmCapture();
        else if (key == 'C') {
            trigger_slope = trigger_slope == SIGNAL_TRIGGER_RISING ?
                SIGNAL_TRIGGER_FALLING : SIGNAL_TRIGGER_RISING;
            display_dirty = true;
        } else if (key == '*') {
            trigger_level = trigger_level > 64U ? trigger_level - 64U : 0U;
            display_dirty = true;
        } else if (key == '#') {
            trigger_level = trigger_level < 4031U ?
                trigger_level + 64U : 4095U;
            display_dirty = true;
        } else if ((key >= '1') && (key <= '3')) {
            pretrigger_percent = key == '1' ? 25U :
                (key == '2' ? 50U : 75U);
            display_dirty = true;
        }
    }
}

/* [READY_PROJECT_LOCAL]
 * 把 CAPTURE_COUNT 个点按屏宽抽取；Y 轴固定 ADC 满量程，避免触发后
 * 自动量程改变导致门限位置看起来漂移。捕获点本身不再重新采集。 */
static void DrawCaptureWaveform(void)
{
    uint32_t x;
    (void)TFT_ST7789_FillRect(&tft, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H,
        TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawRect(&tft, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H,
        TFT_ST7789_BLUE);
    for (x = 1U; x < (uint32_t)(GRAPH_W - 2); ++x) {
        const uint32_t i0 = (x - 1U) * (CAPTURE_COUNT - 1U) /
            (uint32_t)(GRAPH_W - 3);
        const uint32_t i1 = x * (CAPTURE_COUNT - 1U) /
            (uint32_t)(GRAPH_W - 3);
        const int32_t y0 = GRAPH_Y + GRAPH_H - 2 -
            (int32_t)(captured_voltage[i0] * (float)(GRAPH_H - 3) /
            ADC_REFERENCE_V);
        const int32_t y1 = GRAPH_Y + GRAPH_H - 2 -
            (int32_t)(captured_voltage[i1] * (float)(GRAPH_H - 3) /
            ADC_REFERENCE_V);
        (void)TFT_ST7789_DrawLine(&tft, GRAPH_X + (int32_t)x,
            y0, GRAPH_X + (int32_t)x + 1, y1, TFT_ST7789_YELLOW);
    }
    {
        const int32_t trigger_x = GRAPH_X + 1 +
            (int32_t)((uint32_t)(GRAPH_W - 3) * pretrigger_percent / 100U);
        (void)TFT_ST7789_DrawLine(&tft, trigger_x, GRAPH_Y + 1,
            trigger_x, GRAPH_Y + GRAPH_H - 2, TFT_ST7789_RED);
    }
}

static const char *StateName(void)
{
    if (capture_state == CAPTURE_ARM) return "ARM";
    if (capture_state == CAPTURE_WAITING) return "WAITING";
    if (capture_state == CAPTURE_TRIGGERED) return "TRIGGERED";
    return "HOLD";
}

/* ============================================================
 * [函数] UpdateDisplay
 * [功能] 显示状态、触发参数、时间信息、Vpp 和捕获波形。
 * [来源] [FUYONG_ADAPTED] 80/DrawPage + 21/DrawTimeDomainWaveform；
 * 状态排版为 [READY_PROJECT_LOCAL]。
 * [输入] 当前捕获结果；[输出] TFT；[单位] V/us/code/%。
 * [全局] 全部捕获/UI 状态；[步骤] 首次画静态页→局部字段→HOLD 波形区。
 * [原因] 状态变化或捕获完成后才刷新，不占用采集实时路径。
 * [单帧唯一] 每次状态变化一次；[复用] main。
 * [差异] 增加 SINGLE 四状态；[依赖] TFT/font。
 * ============================================================ */
static void UpdateDisplay(void)
{
    const float duration_us = (float)CAPTURE_COUNT * 1000000.0f / sample_rate_hz;
    const float start_us = -(float)CAPTURE_COUNT *
        (float)pretrigger_percent * 10000.0f / sample_rate_hz;
    const float end_us = duration_us + start_us;
    /*
     * [READY_PROJECT_LOCAL]
     * SINGLE 捕获当前只需一个页面；仍显式保存 current_page，
     * 使页面状态只表达“用户正在看什么”，不与 ARM/HOLD 采集状态混用。
     */
    if (current_page != PAGE_CAPTURE) {
        return;
    }
    if (!static_ui_drawn) {
        (void)TFT_ST7789_FillScreen(&tft, TFT_ST7789_BLACK);
        DrawText(8, 4, "TRIGGER BURST CAPTURE", TFT_ST7789_CYAN);
        DrawText(208, 24, "Level:", TFT_ST7789_WHITE);
        DrawText(8, 48, "Dur/Start/End us:", TFT_ST7789_WHITE);
        (void)TFT_ST7789_DrawRect(&tft, GRAPH_X, GRAPH_Y, GRAPH_W,
            GRAPH_H, TFT_ST7789_BLUE);
        DrawText(8, 212, "Vpp:", TFT_ST7789_WHITE);
        DrawText(136, 212, "Pre:", TFT_ST7789_WHITE);
        DrawText(216, 212, "% D=ARM", TFT_ST7789_WHITE);
        static_ui_drawn = true;
    }
    (void)TFT_ST7789_FillRect(&tft, 0, 24, 208, 16, TFT_ST7789_BLACK);
    DrawText(8, 24, StateName(), capture_state == CAPTURE_HOLD ?
        TFT_ST7789_GREEN : TFT_ST7789_YELLOW);
    DrawText(104, 24, trigger_slope == SIGNAL_TRIGGER_RISING ? "RISING" : "FALLING", TFT_ST7789_WHITE);
    (void)TFT_ST7789_FillRect(&tft, 264, 24, 56, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawInt32(&tft, 264, 24, trigger_level,
        TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 152, 48, 168, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 152, 48, duration_us, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 216, 48, start_us, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 272, 48, end_us, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    if (capture_state == CAPTURE_HOLD) DrawCaptureWaveform();
    else (void)TFT_ST7789_FillRect(&tft, GRAPH_X + 1, GRAPH_Y + 1,
        GRAPH_W - 2, GRAPH_H - 2, TFT_ST7789_BLACK);
    (void)TFT_ST7789_FillRect(&tft, 56, 212, 72, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 56, 212, capture_vpp_v, 3U,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 184, 212, 32, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawInt32(&tft, 184, 212, pretrigger_percent,
        TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    display_dirty = false;
}

/* [FUYONG_ADAPTED][moni01] ISR 只把已消抖字符写入 8 项队列。 */
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
    static uint8_t elapsed_ms;
    char symbol;
    ++elapsed_ms;
    if (elapsed_ms < KEYPAD_SCAN_MS) return;
    elapsed_ms = 0U;
    if (SignalMatrixKeypad4x4_ReadNewSymbol(&symbol) == SIGNAL_RESULT_OK) {
        QueueKey(symbol);
    }
}

static void App_Init(void)
{
    const signal_dual_adc_config_t config = {
        SAMPLE_RATE_REQUEST_HZ, CPUCLK_FREQ, 65536U
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
        if (capture_state == CAPTURE_ARM) {
            capture_state = CAPTURE_WAITING; display_dirty = true;
        }
        if (capture_state == CAPTURE_WAITING && AcquireADCFrame() && Trigger_Capture()) {
            capture_state = CAPTURE_TRIGGERED; display_dirty = true;
        }
        if (capture_state == CAPTURE_TRIGGERED) {
            PrepareCaptureResult(); capture_state = CAPTURE_HOLD; display_dirty = true;
        }
        if (display_dirty) UpdateDisplay();
        if (capture_state == CAPTURE_HOLD) __WFI();
    }
}
