/*
 * 工程：moni02 内置 DAC + DDS 可调波形发生器
 *
 * 功能：
 * 1. 内置 DAC 输出正弦波、方波、锯齿波；
 * 2. 4x4 键盘修改 100 Hz~10 kHz 频率和峰峰值；
 * 3. 方波可改占空比，锯齿波可改对称度；
 * 4. ST7789 屏幕显示请求参数和 DDS 实际输出频率。
 *
 * 注释标记：
 * [复制]     函数结构或调用直接来自 fuyong 教学工程；
 * [复制改参] 从 fuyong 复制，只按本题改变缓冲容量、文字或参数；
 * [自己写]   moni02 特有的少量按键状态与界面胶水逻辑。
 */

#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_keypad_number_input.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_wave_output_mspm0g3507.h"

/* ============================================================
 * 第 1 组：DDS / DAC 独立数据
 * 这些变量只服务于波形生成，不兼任界面页号或按键缓存。
 * ============================================================ */

/* [复制改参：fuyong/90_dds_usage]
 * 256 点基础波表由正弦/方波/锯齿波模块生成。
 */
static uint16_t g_wave_table[MONI02_WAVE_TABLE_COUNT];

/* [复制改参：fuyong/90_dds_usage]
 * 100 kHz 更新率输出 100 Hz 时一周期需要 1000 点，所以从示例的 512 点
 * 扩大为 1024 点；否则最低频率会返回 SIGNAL_RESULT_OUT_OF_RANGE。
 */
static uint16_t g_dac_output[MONI02_DAC_OUTPUT_CAPACITY];

/* [复制：fuyong/90_dds_usage]
 * DAC 定时更新率、定时器时钟、16 位计数器最大计数接口保持不变。
 */
static const signal_dac_dma_mspm0_config_t g_dac_config = {
    SIGNAL_DAC_UPDATE_RATE_HZ, CPUCLK_FREQ, 65536U
};

/* [复制：fuyong/90_dds_usage]
 * 每次重新输出后由模块写回实际频率、实际 Vpp 和 DMA 点数。
 */
static signal_wave_output_result_t g_dds_result;

/* ============================================================
 * 第 2 组：用户可调波形参数
 * 方波占空比和锯齿波对称度各有自己的变量，切换波形不会互相覆盖。
 * ============================================================ */
static signal_wave_output_type_t g_wave_type = SIGNAL_WAVE_OUTPUT_SINE;
static float g_frequency_hz = SIGNAL_DDS_FREQUENCY_HZ;
static float g_vpp_v = MONI02_VPP_DEFAULT_V;
static float g_square_duty = MONI02_SQUARE_DUTY_DEFAULT;
static float g_saw_symmetry = MONI02_SAW_SYMMETRY_DEFAULT;

/* ============================================================
 * 第 3 组：界面与直接数字输入状态
 * 只记录当前编辑项、输入文本和范围错误，不参与 DDS 内部计算。
 * ============================================================ */
typedef enum {
    EDIT_FREQUENCY = 0,
    EDIT_VPP,
    EDIT_SHAPE
} edit_parameter_t;

/* [参考 moni01 的 revision/局部刷新思路]
 * 每一位只代表一个小显示区域；按键函数返回哪些区域变脏。
 * APPLY_WAVE 只控制 DDS，不属于显示区域。
 */
typedef uint16_t ui_update_t;

#define UI_UPDATE_NONE             (0U)
#define UI_UPDATE_APPLY_WAVE       (1U << 0)
#define UI_DIRTY_WAVE              (1U << 1)
#define UI_DIRTY_FREQUENCY_SET     (1U << 2)
#define UI_DIRTY_FREQUENCY_OUT     (1U << 3)
#define UI_DIRTY_VPP               (1U << 4)
#define UI_DIRTY_SHAPE             (1U << 5)
#define UI_DIRTY_INPUT             (1U << 6)
#define UI_DIRTY_EDIT              (1U << 7)
#define UI_DIRTY_ALL_DYNAMIC       (UI_DIRTY_WAVE | UI_DIRTY_FREQUENCY_SET | \
    UI_DIRTY_FREQUENCY_OUT | UI_DIRTY_VPP | UI_DIRTY_SHAPE | \
    UI_DIRTY_INPUT | UI_DIRTY_EDIT)

static edit_parameter_t g_edit_parameter = EDIT_FREQUENCY;

/* [复制：fuyong/70_keypad_usage]
 * 数字、小数点、删除、取消、确认全部由独立通用模块保存和解析。
 */
static signal_keypad_number_input_t g_number_input;
static bool g_input_range_error;

/* ============================================================
 * 第 4 组：TFT 和调试状态
 * ============================================================ */
static tft_st7789_t g_tft;

/* 方便 CCS Expressions 观察：0 表示最近一次模块调用成功，其他值是错误码。 */
volatile int32_t g_contest_status;

/* ============================================================
 * [复制改参 START：DDS_INIT]
 * 来源：fuyong/90_dds_usage/main.c -> InitDDSOutput()
 * 修改：只把数组长度换成本工程统一宏，并把参考电压改为 DAC 专用名字。
 * 功能：把基础波表、DMA 输出表、DAC 定时配置交给复用模块。
 * ============================================================ */
static bool InitDDSOutput(void)
{
    const signal_wave_output_config_t config = {
        g_wave_table,
        MONI02_WAVE_TABLE_COUNT,
        g_dac_output,
        MONI02_DAC_OUTPUT_CAPACITY,
        g_dac_config,
        12U,
        SIGNAL_DAC_REFERENCE_V
    };

    g_contest_status = (int32_t)SignalWaveOutput_Init(&config);
    return g_contest_status == (int32_t)SIGNAL_RESULT_OK;
}
/* [复制改参 END：DDS_INIT] */

/* ============================================================
 * [自己写 START：MONI02_APPLY_WAVE]
 * 大模块没有重写：本函数只把本题的 5 个独立参数接到 fuyong 的统一接口。
 * 正弦波不使用 shape；方波传占空比；锯齿波传对称度。
 * ============================================================ */
static bool ApplyWaveform(void)
{
    float shape_fraction = 0.5f;

    if (g_wave_type == SIGNAL_WAVE_OUTPUT_SQUARE) {
        shape_fraction = g_square_duty;
    } else if (g_wave_type == SIGNAL_WAVE_OUTPUT_SAWTOOTH) {
        shape_fraction = g_saw_symmetry;
    }

    g_contest_status = (int32_t)SignalWaveOutput_Start(
        g_wave_type, g_frequency_hz, g_vpp_v,
        SIGNAL_DAC_OFFSET_V, shape_fraction);
    if (g_contest_status != (int32_t)SIGNAL_RESULT_OK) {
        return false;
    }

    g_contest_status = (int32_t)SignalWaveOutput_GetLastResult(&g_dds_result);
    return g_contest_status == (int32_t)SIGNAL_RESULT_OK;
}
/* [自己写 END：MONI02_APPLY_WAVE] */

/* ============================================================
 * [复制 START：KEY_READ]
 * 来源：fuyong/70_keypad_usage/main.c -> ReadKeypad()
 * 功能：只在按键完成消抖并产生一次“新按下”事件时返回 true。
 * ============================================================ */
static bool ReadKeypad(char *key)
{
    return SignalMatrixKeypad4x4_ReadNewSymbol(key) == SIGNAL_RESULT_OK;
}
/* [复制 END：KEY_READ] */

/* ============================================================
 * [复制 START：TFT_INIT]
 * 来源：fuyong/80_tft_usage/main.c -> InitTFTDemo()
 * 旋转角度、偏移量和平台适配接口均保持一致。
 * ============================================================ */
static bool InitTFT(void)
{
    return SignalTFTST7789_MSPM0_Init(&g_tft, TFT_ST7789_ROTATION_270,
        0U, 0U) == TFT_ST7789_OK;
}
/* [复制 END：TFT_INIT] */

/* ============================================================
 * [复制改参 START：TFT_STATIC_TEXT]
 * 来源：fuyong/80_tft_usage/main.c -> DrawStaticText()
 * 修改：标题和按键提示换成本题文字；固定内容仍然只画一次。
 * ============================================================ */
static void DrawStaticText(void)
{
    (void)TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawString(&g_tft, 8, 4, "MONI02 DAC DDS",
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 8, 204,
        "A:WAVE B:EDIT C:RST/CANCEL",
        TFT_ST7789_FONT_6X12, TFT_ST7789_GREEN, TFT_ST7789_BLACK,
        false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 8, 220,
        "0-9:TYPE *:DOT D:DEL #:OK",
        TFT_ST7789_FONT_6X12, TFT_ST7789_GREEN, TFT_ST7789_BLACK,
        false, false);

    /* [参考 moni01/App_DrawStaticUi]
     * 下面这些标签和单位上电只画一次，动态刷新绝不重复画它们。
     */
    (void)TFT_ST7789_DrawString(&g_tft, 8, 32, "WAVE:",
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 8, 56, "F SET:",
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 224, 56, "Hz",
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 8, 80, "F OUT:",
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 224, 80, "Hz",
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 8, 104, "VPP:",
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 224, 104, "V",
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 8, 152, "INPUT:",
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 8, 176, "EDIT:",
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
}
/* [复制改参 END：TFT_STATIC_TEXT] */

/* ============================================================
 * [自己写 START：MONI02_TEXT_HELPERS]
 * 这两个小函数只把枚举变成屏幕文字，不控制硬件。
 * ============================================================ */
static const char *GetWaveName(void)
{
    if (g_wave_type == SIGNAL_WAVE_OUTPUT_SQUARE) {
        return "SQUARE";
    }
    if (g_wave_type == SIGNAL_WAVE_OUTPUT_SAWTOOTH) {
        return "SAW";
    }
    return "SINE";
}

static const char *GetEditName(void)
{
    if (g_edit_parameter == EDIT_VPP) {
        return "VPP";
    }
    if (g_edit_parameter == EDIT_SHAPE) {
        return (g_wave_type == SIGNAL_WAVE_OUTPUT_SQUARE) ? "DUTY" : "SYMM";
    }
    return "FREQ";
}
/* [自己写 END：MONI02_TEXT_HELPERS] */

/* ============================================================
 * [复制改参 START：MONI02_LOCAL_REFRESH]
 * 来源：moni01/main.c 的 App_ClearText() 与 fuyong/80_tft_usage 的
 * UpdateLiveValue()。每个函数只清除并重画自己的字符框。
 * ============================================================ */
static void ClearText(int32_t x, int32_t y, uint8_t character_count)
{
    (void)TFT_ST7789_FillRect(&g_tft, x, y,
        (int32_t)character_count * 8, 16, TFT_ST7789_BLACK);
}

static void DrawWaveField(void)
{
    ClearText(112, 32, 12U);
    (void)TFT_ST7789_DrawString(&g_tft, 112, 32, GetWaveName(),
        TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK,
        false, false);
}

static void DrawFrequencySetField(void)
{
    uint16_t color = (g_edit_parameter == EDIT_FREQUENCY) ?
        TFT_ST7789_YELLOW : TFT_ST7789_CYAN;

    ClearText(112, 56, 12U);
    (void)TFT_ST7789_DrawFloat(&g_tft, 112, 56, g_frequency_hz, 1U,
        TFT_ST7789_FONT_8X16, color, TFT_ST7789_BLACK, false);
}

static void DrawFrequencyOutField(void)
{
    ClearText(112, 80, 12U);
    (void)TFT_ST7789_DrawFloat(&g_tft, 112, 80,
        g_dds_result.actual_frequency_hz, 1U,
        TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
}

static void DrawVppField(void)
{
    uint16_t color = (g_edit_parameter == EDIT_VPP) ?
        TFT_ST7789_YELLOW : TFT_ST7789_CYAN;

    ClearText(112, 104, 12U);
    (void)TFT_ST7789_DrawFloat(&g_tft, 112, 104, g_vpp_v, 1U,
        TFT_ST7789_FONT_8X16, color, TFT_ST7789_BLACK, false);
}

static void DrawShapeField(void)
{
    uint16_t color = (g_edit_parameter == EDIT_SHAPE) ?
        TFT_ST7789_YELLOW : TFT_ST7789_CYAN;

    /* 标签会在 SHAPE/DUTY/SYMM 间变化，因此标签和值分别局部清除。 */
    ClearText(8, 128, 8U);
    ClearText(112, 128, 16U);
    if (g_wave_type == SIGNAL_WAVE_OUTPUT_SINE) {
        (void)TFT_ST7789_DrawString(&g_tft, 8, 128, "SHAPE:",
            TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
            false, false);
        (void)TFT_ST7789_DrawString(&g_tft, 112, 128, "N/A",
            TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK,
            false, false);
    } else {
        const char *label = (g_wave_type == SIGNAL_WAVE_OUTPUT_SQUARE) ?
            "DUTY:" : "SYMM:";
        float percent = (g_wave_type == SIGNAL_WAVE_OUTPUT_SQUARE) ?
            g_square_duty * 100.0f : g_saw_symmetry * 100.0f;

        (void)TFT_ST7789_DrawString(&g_tft, 8, 128, label,
            TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
            false, false);
        (void)TFT_ST7789_DrawFloat(&g_tft, 112, 128, percent, 0U,
            TFT_ST7789_FONT_8X16, color, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_DrawString(&g_tft, 224, 128, "%",
            TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
            false, false);
    }
}

static void DrawInputField(void)
{
    ClearText(112, 152, 16U);
    if (SignalKeypadNumberInput_IsActive(&g_number_input)) {
        const char *text = SignalKeypadNumberInput_GetText(&g_number_input);

        (void)TFT_ST7789_DrawString(&g_tft, 112, 152,
            (text[0] == '\0') ? "_" : text,
            TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK,
            false, false);
    } else if (g_input_range_error) {
        (void)TFT_ST7789_DrawString(&g_tft, 112, 152, "RANGE ERROR",
            TFT_ST7789_FONT_8X16, TFT_ST7789_RED, TFT_ST7789_BLACK,
            false, false);
    } else {
        (void)TFT_ST7789_DrawString(&g_tft, 112, 152, "READY",
            TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK,
            false, false);
    }
}

static void DrawEditField(void)
{
    ClearText(112, 176, 12U);
    (void)TFT_ST7789_DrawString(&g_tft, 112, 176, GetEditName(),
        TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK,
        false, false);
}

/* 只调用 dirty_mask 指定的字段函数，不再清除整个参数面板。 */
static void DrawDirtyFields(ui_update_t dirty_mask)
{
    if ((dirty_mask & UI_DIRTY_WAVE) != 0U) DrawWaveField();
    if ((dirty_mask & UI_DIRTY_FREQUENCY_SET) != 0U) {
        DrawFrequencySetField();
    }
    if ((dirty_mask & UI_DIRTY_FREQUENCY_OUT) != 0U) {
        DrawFrequencyOutField();
    }
    if ((dirty_mask & UI_DIRTY_VPP) != 0U) DrawVppField();
    if ((dirty_mask & UI_DIRTY_SHAPE) != 0U) DrawShapeField();
    if ((dirty_mask & UI_DIRTY_INPUT) != 0U) DrawInputField();
    if ((dirty_mask & UI_DIRTY_EDIT) != 0U) DrawEditField();
}
/* [复制改参 END：MONI02_LOCAL_REFRESH] */

/* ============================================================
 * [自己写 START：MONI02_PARAMETER_LOGIC]
 * 通用模块负责输入文本和十进制解析；这里仅赋予数值单位、检查本题范围，
 * 再切换波形/编辑项。此区域不访问 DAC、DMA、SPI 或 GPIO 寄存器。
 * ============================================================ */
static bool IsValueInRange(float value, float minimum, float maximum)
{
    return (value >= minimum) && (value <= maximum);
}

static void SelectNextWave(void)
{
    if (g_wave_type == SIGNAL_WAVE_OUTPUT_SINE) {
        g_wave_type = SIGNAL_WAVE_OUTPUT_SQUARE;
    } else if (g_wave_type == SIGNAL_WAVE_OUTPUT_SQUARE) {
        g_wave_type = SIGNAL_WAVE_OUTPUT_SAWTOOTH;
    } else {
        g_wave_type = SIGNAL_WAVE_OUTPUT_SINE;
        if (g_edit_parameter == EDIT_SHAPE) {
            g_edit_parameter = EDIT_FREQUENCY;
        }
    }
}

static void SelectNextParameter(void)
{
    if (g_wave_type == SIGNAL_WAVE_OUTPUT_SINE) {
        g_edit_parameter = (g_edit_parameter == EDIT_FREQUENCY) ?
            EDIT_VPP : EDIT_FREQUENCY;
    } else {
        g_edit_parameter = (edit_parameter_t)(((uint32_t)g_edit_parameter + 1U) % 3U);
    }
}

/*
 * 把刚按 # 确认的数字写入当前编辑参数。
 * 频率单位是 Hz，Vpp 单位是 V，DUTY/SYMM 的输入单位是百分数。
 */
static bool CommitConfirmedNumber(void)
{
    float entered_value;

    if (SignalKeypadNumberInput_GetValue(
            &g_number_input, &entered_value) != SIGNAL_RESULT_OK) {
        return false;
    }

    if (g_edit_parameter == EDIT_FREQUENCY) {
        if (!IsValueInRange(entered_value,
                MONI02_FREQUENCY_MIN_HZ, MONI02_FREQUENCY_MAX_HZ)) {
            return false;
        }
        g_frequency_hz = entered_value;
        return true;
    }

    if (g_edit_parameter == EDIT_VPP) {
        if (!IsValueInRange(
                entered_value, MONI02_VPP_MIN_V, MONI02_VPP_MAX_V)) {
            return false;
        }
        g_vpp_v = entered_value;
        return true;
    }

    if (g_wave_type == SIGNAL_WAVE_OUTPUT_SQUARE) {
        if (!IsValueInRange(entered_value,
                MONI02_SQUARE_DUTY_MIN * 100.0f,
                MONI02_SQUARE_DUTY_MAX * 100.0f)) {
            return false;
        }
        g_square_duty = entered_value / 100.0f;
        return true;
    }

    if (g_wave_type == SIGNAL_WAVE_OUTPUT_SAWTOOTH) {
        if (!IsValueInRange(entered_value,
                MONI02_SAW_SYMMETRY_MIN * 100.0f,
                MONI02_SAW_SYMMETRY_MAX * 100.0f)) {
            return false;
        }
        g_saw_symmetry = entered_value / 100.0f;
        return true;
    }

    return false;
}

static void ResetParameters(void)
{
    g_wave_type = SIGNAL_WAVE_OUTPUT_SINE;
    g_frequency_hz = SIGNAL_DDS_FREQUENCY_HZ;
    g_vpp_v = MONI02_VPP_DEFAULT_V;
    g_square_duty = MONI02_SQUARE_DUTY_DEFAULT;
    g_saw_symmetry = MONI02_SAW_SYMMETRY_DEFAULT;
    g_edit_parameter = EDIT_FREQUENCY;
    g_input_range_error = false;
    (void)SignalKeypadNumberInput_Init(&g_number_input);
}

static ui_update_t GetEditedFieldDirtyMask(void)
{
    if (g_edit_parameter == EDIT_FREQUENCY) {
        return UI_DIRTY_FREQUENCY_SET;
    }
    if (g_edit_parameter == EDIT_VPP) {
        return UI_DIRTY_VPP;
    }
    return UI_DIRTY_SHAPE;
}

static ui_update_t HandleKey(char key)
{
    bool is_digit = (key >= '0') && (key <= '9');

    /*
     * 输入已开始时，数字、*、D、C、# 全部交给复用模块。
     * 空闲时按第一个数字或 * 会自动清空旧文本并开始新输入。
     */
    if (SignalKeypadNumberInput_IsActive(&g_number_input) ||
        is_digit || (key == '*')) {
        signal_keypad_number_input_event_t event;
        signal_result_t result = SignalKeypadNumberInput_HandleKey(
            &g_number_input, key, &event);

        if (result == SIGNAL_RESULT_INSUFFICIENT_BUFFER) {
            g_input_range_error = true;
            return UI_DIRTY_INPUT;
        }
        if (result != SIGNAL_RESULT_OK) {
            return UI_UPDATE_NONE;
        }
        if (event == SIGNAL_KEYPAD_NUMBER_INPUT_CONFIRMED) {
            if (CommitConfirmedNumber()) {
                g_input_range_error = false;
                return UI_UPDATE_APPLY_WAVE | UI_DIRTY_INPUT |
                    GetEditedFieldDirtyMask();
            }
            g_input_range_error = true;
            return UI_DIRTY_INPUT;
        }
        if ((event == SIGNAL_KEYPAD_NUMBER_INPUT_UPDATED) ||
            (event == SIGNAL_KEYPAD_NUMBER_INPUT_CANCELLED)) {
            g_input_range_error = false;
            return UI_DIRTY_INPUT;
        }
        return UI_UPDATE_NONE;
    }

    if (key == 'A') {
        SelectNextWave();
        g_input_range_error = false;
        return UI_UPDATE_APPLY_WAVE | UI_DIRTY_WAVE |
            UI_DIRTY_FREQUENCY_SET | UI_DIRTY_VPP | UI_DIRTY_SHAPE |
            UI_DIRTY_INPUT | UI_DIRTY_EDIT;
    }
    if (key == 'B') {
        SelectNextParameter();
        g_input_range_error = false;
        return UI_DIRTY_FREQUENCY_SET | UI_DIRTY_VPP |
            UI_DIRTY_SHAPE | UI_DIRTY_INPUT | UI_DIRTY_EDIT;
    }
    if (key == 'C') {
        ResetParameters();
        return UI_UPDATE_APPLY_WAVE | UI_DIRTY_ALL_DYNAMIC;
    }
    return UI_UPDATE_NONE;
}
/* [自己写 END：MONI02_PARAMETER_LOGIC] */

int main(void)
{
    /* [复制：所有 fuyong MSPM0 工程] 先初始化 SysConfig 生成的硬件。 */
    SYSCFG_DL_init();

    /* [复制组合：90_dds_usage + 80_tft_usage]
     * 初始化失败时停止在这里，CCS 观察 g_contest_status 可定位 DDS 错误。
     */
    if ((SignalKeypadNumberInput_Init(&g_number_input) != SIGNAL_RESULT_OK) ||
        !InitDDSOutput() || !InitTFT() || !ApplyWaveform()) {
        while (true) {
        }
    }

    DrawStaticText();
    DrawDirtyFields(UI_DIRTY_ALL_DYNAMIC);

    while (true) {
        char key;

        /* [复制：70_keypad_usage 的周期读取方式]
         * 键盘模块要求约每 5 ms 扫描一次；这里不使用 __WFI()，避免没有中断时
         * 主循环永久睡眠，也不会因为循环过快让三次消抖失去时间意义。
         */
        delay_cycles(CPUCLK_FREQ / 200U);
        if (ReadKeypad(&key)) {
            ui_update_t update = HandleKey(key);

            if ((update & UI_UPDATE_APPLY_WAVE) != 0U) {
                if (ApplyWaveform()) {
                    update |= UI_DIRTY_FREQUENCY_OUT;
                }
            }
            DrawDirtyFields(update & UI_DIRTY_ALL_DYNAMIC);
        }
    }
}
