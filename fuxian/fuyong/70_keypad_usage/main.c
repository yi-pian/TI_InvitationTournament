/* 工程：70_keypad_usage。教学流程：读新按键 → 页面切换/数字输入/参数调整。 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_keypad_number_input.h"
#include "signal_matrix_keypad_4x4.h"

/* 当前 UI 页号；HandlePageSwitch() 写入，页面绘制代码读取。 */
static uint8_t current_page;
/* 用户可调示例参数，float；单位由使用它的功能定义，本例初值为 1000.0。 */
static float adjustable_value = 1000.0f;
/* NUMBER_INPUT 的通用输入对象；独立保存文本、长度、小数点和确认值。 */
static signal_keypad_number_input_t number_input;

/* ============================================================
 * [COPY START: KEY_READ]
 * 函数：ReadKeypad
 * [功能] 从 4×4 矩阵键盘读取一个“新按下”的符号，避免长按重复触发。
 * [输入] 键盘硬件和已由 SysConfig 初始化的平台扫描资源。
 * [输出] key：char；仅返回 true 时有效。
 * [返回值] true：得到一次新的按键事件；false：无新按键或驱动未给出事件。
 * [复用] 需要 signal_matrix_keypad_4x4.c/.h；主循环应周期性调用而不是只在
 * 页面刷新时调用。
 * ============================================================ */
static bool ReadKeypad(char *key)
{
    return SignalMatrixKeypad4x4_ReadNewSymbol(key) == SIGNAL_RESULT_OK;
}
/* [COPY END: KEY_READ] */

/* ============================================================
 * [COPY START: PAGE_SWITCH]
 * 函数：HandlePageSwitch
 * [功能] 处理 A/B 键并循环切换 current_page。
 * [输入] key：ReadKeypad() 得到的 ASCII 按键。
 * [输出] current_page：uint8_t；0/1 表示示例的两页。
 * [为什么独立] 页面语义与数字输入、参数调整无关；综合工程可替换页数但保留边界。
 * [复用] 复制本函数和 current_page；改变页面数量时同步修改取模值。
 * ============================================================ */
static void HandlePageSwitch(char key)
{
    if (key == 'A' || key == 'B') {
        current_page = (uint8_t)((current_page + 1U) % 2U);
    }
}
/* [COPY END: PAGE_SWITCH] */

/* ============================================================
 * [COPY START: NUMBER_INPUT]
 * 函数：HandleNumberInput
 * [功能] 使用通用模块接收直接数字输入，确认后写入 adjustable_value。
 * [输入] key：单次键盘事件；number_input：独立输入状态。
 * [输出] 输入中可用 SignalKeypadNumberInput_GetText() 显示；确认后得到 float。
 * [按键语义] 0~9 追加；* 小数点；D 删除；C 取消；# 确认。
 * [为什么独立] 文本、长度、小数点和确认值由模块管理，不借用参数变量做缓存。
 * [复用] 复制 signal_keypad_number_input.c/.h；题目代码在确认后检查自己的范围。
 * ============================================================ */
static void HandleNumberInput(char key)
{
    signal_keypad_number_input_event_t event;

    if ((SignalKeypadNumberInput_HandleKey(
            &number_input, key, &event) == SIGNAL_RESULT_OK) &&
        (event == SIGNAL_KEYPAD_NUMBER_INPUT_CONFIRMED)) {
        (void)SignalKeypadNumberInput_GetValue(
            &number_input, &adjustable_value);
    }
}
/* [COPY END: NUMBER_INPUT] */

/* ============================================================
 * [COPY START: PARAMETER_ADJUST]
 * 函数：HandleParameterAdjust
 * [功能] 用星号键和井号键对 adjustable_value 做 10.0 的减/加，且保持原有下限 10.0。
 * [输入] key；[输出] adjustable_value。
 * [单位] 本例不强制单位；若用于 DDS 频率则为 Hz，若用于阈值则必须改注释和边界。
 * [复用] 复制本函数与 adjustable_value；不要把该变量同时用于输入文本或页号。
 * ============================================================ */
static void HandleParameterAdjust(char key)
{
    if (key == '*' && adjustable_value > 10.0f) {
        adjustable_value -= 10.0f;
    } else if (key == '#') {
        adjustable_value += 10.0f;
    }
}
/* [COPY END: PARAMETER_ADJUST] */

int main(void)
{
    SYSCFG_DL_init();
    (void)SignalKeypadNumberInput_Init(&number_input);

    while (true) {
        char key;

        if (!ReadKeypad(&key)) {
            __WFI();
            continue;
        }
        if (SignalKeypadNumberInput_IsActive(&number_input) ||
            ((key >= '0') && (key <= '9'))) {
            /* 输入期间独占按键：* 小数点、D 删除、C 取消、# 确认。 */
            HandleNumberInput(key);
        } else {
            HandlePageSwitch(key);
            HandleParameterAdjust(key);
        }
    }
}
