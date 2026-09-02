/* ============================================================
 * 工程：03_programmable_signal_generator
 * 用途：片内 DAC/DDS 可编程信号发生器。
 * 输出：DAC0/PA15；显示：ST7789；输入：4x4 键盘。
 *
 * KEY MAP
 * A/B：上/下一个波形；C：下一个参数；D 或 #：确认并输出；
 * 数字：输入当前参数；*：删除，空输入时减小；#：空输入时增大。
 *
 * 来源：fuyong/90_dds_usage、70_keypad_usage、80_tft_usage；
 * 平台闭包来自已验证 example04。经授权复制 modules/.syscfg，未改模块。
 * ============================================================ */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_dac_dma_mspm0g3507.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_wave_output_mspm0g3507.h"

#define DDS_TABLE_COUNT             (256U)
#define DDS_OUTPUT_COUNT            (1024U)
#define DDS_UPDATE_RATE_HZ          (100000U)
#define DDS_REFERENCE_V             (3.3f)
#define INPUT_DIGITS                (7U)
#define KEYPAD_SCAN_PERIOD_MS       (5U)
#define KEY_QUEUE_SIZE              (8U)

typedef enum { WAVE_SINE = 0U, WAVE_TRIANGLE, WAVE_SAWTOOTH,
    WAVE_SQUARE, WAVE_COUNT } waveform_t;
typedef enum { PARAM_FREQUENCY = 0U, PARAM_VPP, PARAM_OFFSET,
    PARAM_DUTY, PARAM_SYMMETRY, PARAM_COUNT } parameter_t;
typedef enum { PAGE_GENERATOR = 0U } app_page_t;

static uint16_t dds_wave_table[DDS_TABLE_COUNT];
static uint16_t dds_output_buffer[DDS_OUTPUT_COUNT];
static tft_st7789_t tft;
static waveform_t current_waveform = WAVE_SINE;
static app_page_t current_page = PAGE_GENERATOR;
static parameter_t selected_parameter = PARAM_FREQUENCY;
static float target_frequency_hz = 1000.0f;
static float target_vpp_v = 1.0f;
static float target_offset_v = 1.65f;
static float target_duty_percent = 50.0f;
static float target_symmetry_percent = 50.0f;
static char number_input[INPUT_DIGITS + 1U];
static uint8_t number_length;
static volatile char key_queue[KEY_QUEUE_SIZE];
static volatile uint8_t key_queue_head;
static volatile uint8_t key_queue_tail;
static bool output_dirty = true;
static bool display_dirty = true;
static bool static_ui_drawn;
static signal_result_t output_status = SIGNAL_RESULT_OK;

/* ============================================================
 * [函数] DrawText
 * [功能] 统一使用 8x16 字体绘制 ASCII。
 * [来源] [FUYONG_ADAPTED] 80_tft_usage/DrawStaticText。
 * [输入] 像素坐标、字符串、RGB565；[输出] TFT 像素。
 * [单位] x/y：pixel；[全局] tft。
 * [步骤/原因] 包装字库 API，避免页面重复底层参数。
 * [单帧唯一] 否；只在页面更新时调用。
 * [复用] 所有页面字段；[差异] 仅参数化。
 * [依赖] signal_tft_st7789_font。
 * ============================================================ */
static void DrawText(int32_t x, int32_t y, const char *text, uint16_t color)
{
    (void)TFT_ST7789_DrawString(&tft, x, y, text, TFT_ST7789_FONT_8X16,
        color, TFT_ST7789_BLACK, false, false);
}

/* [READY_PROJECT_LOCAL] 七位十进制输入解析；不使用 scanf，行为可预测。 */
static bool ParseNumber(uint32_t *value)
{
    uint8_t index;
    uint32_t parsed = 0U;
    if ((value == NULL) || (number_length == 0U)) return false;
    for (index = 0U; index < number_length; ++index) {
        parsed = parsed * 10U + (uint32_t)(number_input[index] - '0');
    }
    *value = parsed;
    return true;
}

/* [READY_PROJECT_LOCAL] 保证 offset±Vpp/2 落在 DAC 的 0~3.3 V 内。 */
static void ClampOutputParameters(void)
{
    float maximum_vpp;
    if (target_frequency_hz < 100.0f) target_frequency_hz = 100.0f;
    if (target_frequency_hz > 20000.0f) target_frequency_hz = 20000.0f;
    if (target_offset_v < 0.05f) target_offset_v = 0.05f;
    if (target_offset_v > 3.25f) target_offset_v = 3.25f;
    maximum_vpp = 2.0f * ((target_offset_v < DDS_REFERENCE_V - target_offset_v) ?
        target_offset_v : DDS_REFERENCE_V - target_offset_v);
    if (target_vpp_v < 0.02f) target_vpp_v = 0.02f;
    if (target_vpp_v > maximum_vpp) target_vpp_v = maximum_vpp;
    if (target_duty_percent < 1.0f) target_duty_percent = 1.0f;
    if (target_duty_percent > 99.0f) target_duty_percent = 99.0f;
    if (target_symmetry_percent < 1.0f) target_symmetry_percent = 1.0f;
    if (target_symmetry_percent > 100.0f) target_symmetry_percent = 100.0f;
}

/* ============================================================
 * [函数] ApplyWaveform
 * [功能] 重新生成波表/DDS 缓冲并启动 DAC DMA。
 * [来源] [FUYONG_ADAPTED] 90_dds_usage/SetDDSFrequency。
 * [输入] target_* 与 current_waveform；[输出] DAC0、output_status。
 * [单位] Hz/V/%；[全局] 波形参数、output_dirty。
 * [步骤] 限幅→选择 Wave Output API→记录状态。
 * [原因] 只改变量不会改变正在播放的 DMA 缓冲。
 * [单帧唯一] 每次参数改变只执行一次；[复用] 上电和确认输入。
 * [差异] 增加四波形分派，模块算法不变。
 * [依赖] signal_wave_output_mspm0g3507。
 * ============================================================ */
static void ApplyWaveform(void)
{
    ClampOutputParameters();
    if (current_waveform == WAVE_SINE) {
        output_status = SignalWaveOutput_SineWithOffset(target_frequency_hz,
            target_vpp_v, target_offset_v);
    } else if (current_waveform == WAVE_SQUARE) {
        output_status = SignalWaveOutput_SquareWithDuty(target_frequency_hz,
            target_vpp_v, target_offset_v, target_duty_percent / 100.0f);
    } else {
        /* SawtoothWithSymmetry 在 50% 时是真三角波；其它比例为非对称三角/锯齿。 */
        output_status = SignalWaveOutput_SawtoothWithSymmetry(
            target_frequency_hz, target_vpp_v, target_offset_v,
            target_symmetry_percent / 100.0f);
    }
    output_dirty = false;
    display_dirty = true;
}

/* [READY_PROJECT_LOCAL] 频率用 Hz，Vpp/Offset 输入用 mV，形状参数用 %。 */
static void CommitNumberInput(void)
{
    uint32_t value;
    if (!ParseNumber(&value)) return;
    if (selected_parameter == PARAM_FREQUENCY) target_frequency_hz = (float)value;
    else if (selected_parameter == PARAM_VPP) target_vpp_v = (float)value / 1000.0f;
    else if (selected_parameter == PARAM_OFFSET) target_offset_v = (float)value / 1000.0f;
    else if (selected_parameter == PARAM_DUTY) target_duty_percent = (float)value;
    else target_symmetry_percent = (float)value;
    number_length = 0U;
    number_input[0] = '\0';
    output_dirty = true;
}

/* [READY_PROJECT_LOCAL] 空输入时做小步进现场微调。 */
static void AdjustSelectedParameter(bool increase)
{
    const float sign = increase ? 1.0f : -1.0f;
    if (selected_parameter == PARAM_FREQUENCY) target_frequency_hz += sign * 100.0f;
    else if (selected_parameter == PARAM_VPP) target_vpp_v += sign * 0.1f;
    else if (selected_parameter == PARAM_OFFSET) target_offset_v += sign * 0.05f;
    else if (selected_parameter == PARAM_DUTY) target_duty_percent += sign;
    else target_symmetry_percent += sign;
    output_dirty = true;
}

/* ============================================================
 * [函数] HandleKeypad
 * [功能] 消费一个已消抖按键，更新波形、参数选择或数字缓存。
 * [来源] [FUYONG_ADAPTED] 70_keypad_usage/HandleNumberInput；
 * 页面语义为 [READY_PROJECT_LOCAL]。
 * [输入] key_queue；[输出] UI/输出参数与 dirty 标志。
 * [单位] 按当前参数；[全局] 键盘和参数状态。
 * [步骤] 取键并清 pending，再分派 A/B/C/D、数字键、星号键和井号键。
 * [原因] 中断不执行 SPI 刷屏或 DMA 重启。
 * [单帧唯一] 每个新按键仅消费一次；[复用] main 首步骤。
 * [差异] 五参数与波形选择；[依赖] 4x4 keypad。
 * ============================================================ */
static void HandleKeypad(void)
{
    char key;
    while (key_queue_tail != key_queue_head) {
        key = key_queue[key_queue_tail];
        key_queue_tail = (uint8_t)((key_queue_tail + 1U) % KEY_QUEUE_SIZE);
        if (key == 'A') {
            current_waveform = (current_waveform == WAVE_SINE) ?
                (waveform_t)(WAVE_COUNT - 1U) :
                (waveform_t)(current_waveform - 1U);
            output_dirty = true;
        } else if (key == 'B') {
            current_waveform =
                (waveform_t)((current_waveform + 1U) % WAVE_COUNT);
            output_dirty = true;
        } else if (key == 'C') {
            selected_parameter =
                (parameter_t)((selected_parameter + 1U) % PARAM_COUNT);
            number_length = 0U;
            display_dirty = true;
        } else if ((key >= '0') && (key <= '9') &&
            (number_length < INPUT_DIGITS)) {
            number_input[number_length++] = key;
            number_input[number_length] = '\0';
            display_dirty = true;
        } else if (key == '*') {
            if (number_length > 0U) {
                number_input[--number_length] = '\0';
                display_dirty = true;
            } else AdjustSelectedParameter(false);
        } else if ((key == '#') || (key == 'D')) {
            if (number_length > 0U) CommitNumberInput();
            else if (key == '#') AdjustSelectedParameter(true);
            else output_dirty = true;
        }
    }
}

static const char *WaveName(void)
{
    if (current_waveform == WAVE_SINE) return "SINE";
    if (current_waveform == WAVE_TRIANGLE) return "TRIANGLE";
    if (current_waveform == WAVE_SAWTOOTH) return "SAWTOOTH";
    return "SQUARE";
}

/* ============================================================
 * [函数] UpdateDisplay
 * [功能] 显示工程名、波形、五参数、输入缓存和运行状态。
 * [来源] [FUYONG_ADAPTED] 80_tft_usage/DrawPage；排版为
 * [READY_PROJECT_LOCAL]。
 * [输入] UI/输出状态；[输出] TFT；[单位] Hz/V/%。
 * [全局] tft 与全部状态；[步骤] 首次画静态页→逐行局部清除并重画动态字段。
 * [原因] 参数变化后刷新一次，避免无意义占用 SPI。
 * [单帧唯一] 是（每次状态变化）；[复用] main。
 * [差异] 无意义参数显示 --；[依赖] ST7789/font。
 * ============================================================ */
static void UpdateDisplay(void)
{
    const uint16_t normal = TFT_ST7789_WHITE;
    const uint16_t selected = TFT_ST7789_YELLOW;
    /*
     * [READY_PROJECT_LOCAL]
     * 本工程目前只有一个仪器页面，但仍保留统一的 current_page 状态。
     * 这样比赛现场若增加帮助页或预置页，只需扩展枚举与此分派点，
     * 不必改动 DDS 参数处理和输出链。
     */
    if (current_page != PAGE_GENERATOR) {
        return;
    }
    if (!static_ui_drawn) {
        /* 上电只画一次固定标题和按键提示；普通参数变化不再整屏清除。 */
        (void)TFT_ST7789_FillScreen(&tft, TFT_ST7789_BLACK);
        DrawText(8, 4, "PROGRAMMABLE GENERATOR", TFT_ST7789_CYAN);
        DrawText(8, 224, "A/B Wave C Param */# -/+", normal);
        static_ui_drawn = true;
    }
    (void)TFT_ST7789_FillRect(&tft, 0, 28, 320, 16, TFT_ST7789_BLACK);
    DrawText(8, 28, "Wave:", normal); DrawText(96, 28, WaveName(), TFT_ST7789_GREEN);
    (void)TFT_ST7789_FillRect(&tft, 0, 52, 320, 16, TFT_ST7789_BLACK);
    DrawText(8, 52, "Frequency:", selected_parameter == PARAM_FREQUENCY ? selected : normal);
    (void)TFT_ST7789_DrawFloat(&tft, 152, 52, target_frequency_hz, 1U,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    DrawText(264, 52, "Hz", normal);
    (void)TFT_ST7789_FillRect(&tft, 0, 76, 320, 16, TFT_ST7789_BLACK);
    DrawText(8, 76, "Vpp:", selected_parameter == PARAM_VPP ? selected : normal);
    (void)TFT_ST7789_DrawFloat(&tft, 152, 76, target_vpp_v, 3U,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    DrawText(248, 76, "V", normal);
    (void)TFT_ST7789_FillRect(&tft, 0, 100, 320, 16, TFT_ST7789_BLACK);
    DrawText(8, 100, "Offset:", selected_parameter == PARAM_OFFSET ? selected : normal);
    (void)TFT_ST7789_DrawFloat(&tft, 152, 100, target_offset_v, 3U,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    DrawText(248, 100, "V", normal);
    (void)TFT_ST7789_FillRect(&tft, 0, 124, 320, 16, TFT_ST7789_BLACK);
    DrawText(8, 124, "Duty:", selected_parameter == PARAM_DUTY ? selected : normal);
    if (current_waveform == WAVE_SQUARE) {
        (void)TFT_ST7789_DrawFloat(&tft, 152, 124, target_duty_percent, 1U,
            TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
        DrawText(248, 124, "%", normal);
    } else DrawText(152, 124, "--", TFT_ST7789_RGB565(100U,100U,100U));
    (void)TFT_ST7789_FillRect(&tft, 0, 148, 320, 16, TFT_ST7789_BLACK);
    DrawText(8, 148, "Symmetry:", selected_parameter == PARAM_SYMMETRY ? selected : normal);
    if ((current_waveform == WAVE_TRIANGLE) || (current_waveform == WAVE_SAWTOOTH)) {
        (void)TFT_ST7789_DrawFloat(&tft, 152, 148, target_symmetry_percent, 1U,
            TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
        DrawText(248, 148, "%", normal);
    } else DrawText(152, 148, "--", TFT_ST7789_RGB565(100U,100U,100U));
    (void)TFT_ST7789_FillRect(&tft, 0, 180, 320, 16, TFT_ST7789_BLACK);
    DrawText(8, 180, "Input:", normal);
    DrawText(96, 180, number_length == 0U ? "--" : number_input, selected);
    (void)TFT_ST7789_FillRect(&tft, 0, 204, 320, 16, TFT_ST7789_BLACK);
    DrawText(8, 204, output_status == SIGNAL_RESULT_OK ? "OUTPUT: RUN" : "OUTPUT: ERROR",
        output_status == SIGNAL_RESULT_OK ? TFT_ST7789_GREEN : TFT_ST7789_RED);
    display_dirty = false;
}

/* [FUYONG_ADAPTED][moni01] 中断只入队，主循环 HandleKeypad 再处理。 */
static void QueueKey(char symbol)
{
    const uint8_t next = (uint8_t)((key_queue_head + 1U) % KEY_QUEUE_SIZE);
    if (next != key_queue_tail) {
        key_queue[key_queue_head] = symbol;
        key_queue_head = next;
    }
}

/* [FUYONG_ADAPTED][moni01] 每 5 ms 扫描；ISR 只把字符放入环形队列。 */
void SysTick_Handler(void)
{
    static uint8_t elapsed_ms;
    char symbol;
    ++elapsed_ms;
    if (elapsed_ms < KEYPAD_SCAN_PERIOD_MS) return;
    elapsed_ms = 0U;
    if (SignalMatrixKeypad4x4_ReadNewSymbol(&symbol) == SIGNAL_RESULT_OK) {
        QueueKey(symbol);
    }
}

/* ============================================================
 * [函数] App_Init
 * [功能] 初始化 SysConfig、Wave Output、TFT、SysTick。
 * [来源] [FUYONG_ADAPTED] 90/InitDDSOutput + 80/InitTFTDemo。
 * [输入] 无；[输出] 外设就绪；[单位] update rate：Hz。
 * [全局] 波表/输出缓冲/tft；[步骤] SysConfig→DDS→TFT→tick。
 * [原因] 顺序与 example04 已验证工程一致；[单帧唯一] 上电一次。
 * [复用] main；[差异] 合并初始化入口；[依赖] 已复制平台模块。
 * ============================================================ */
static void App_Init(void)
{
    const signal_dac_dma_mspm0_config_t dac_config = {
        DDS_UPDATE_RATE_HZ, CPUCLK_FREQ, 65536U
    };
    const signal_wave_output_config_t wave_config = {
        dds_wave_table, DDS_TABLE_COUNT, dds_output_buffer, DDS_OUTPUT_COUNT,
        dac_config, 12U, DDS_REFERENCE_V
    };
    SYSCFG_DL_init();
    if (SignalWaveOutput_Init(&wave_config) != SIGNAL_RESULT_OK) while (true) { }
    if (SignalTFTST7789_MSPM0_Init(&tft, TFT_ST7789_ROTATION_270,
            0U, 0U) != TFT_ST7789_OK) while (true) { }
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (true) { }
}

int main(void)
{
    App_Init();
    while (true) {
        HandleKeypad();
        if (output_dirty) ApplyWaveform();
        if (display_dirty) UpdateDisplay();
        __WFI();
    }
}
