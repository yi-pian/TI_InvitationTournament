#ifndef SIGNAL_KEYPAD_NUMBER_INPUT_H
#define SIGNAL_KEYPAD_NUMBER_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "signal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 最多保存 8 个可见字符，另留 1 字节给字符串结尾 '\0'。 */
#define SIGNAL_KEYPAD_NUMBER_INPUT_CAPACITY (8U)

typedef enum {
    SIGNAL_KEYPAD_NUMBER_INPUT_NO_EVENT = 0,
    SIGNAL_KEYPAD_NUMBER_INPUT_UPDATED,
    SIGNAL_KEYPAD_NUMBER_INPUT_CONFIRMED,
    SIGNAL_KEYPAD_NUMBER_INPUT_CANCELLED
} signal_keypad_number_input_event_t;

typedef struct {
    char text[SIGNAL_KEYPAD_NUMBER_INPUT_CAPACITY + 1U];
    float confirmed_value;
    uint8_t length;
    bool decimal_present;
    bool active;
    bool has_confirmed_value;
} signal_keypad_number_input_t;

/* 初始化为空闲状态；第一次按数字或小数点时自动开始一轮新输入。 */
signal_result_t SignalKeypadNumberInput_Init(
    signal_keypad_number_input_t *input);

/*
 * 通用 4×4 键盘数字输入约定：
 * 0~9：追加数字；*：小数点；D：删除；C：取消；#：确认。
 * A/B 等其他键返回 SIGNAL_RESULT_NO_DATA，不改变输入状态。
 */
signal_result_t SignalKeypadNumberInput_HandleKey(
    signal_keypad_number_input_t *input, char key,
    signal_keypad_number_input_event_t *event);

/* 确认成功后读取解析完成的非负十进制数值。 */
signal_result_t SignalKeypadNumberInput_GetValue(
    const signal_keypad_number_input_t *input, float *value);

/* 返回始终以 '\0' 结尾的显示字符串；无效对象返回空字符串。 */
const char *SignalKeypadNumberInput_GetText(
    const signal_keypad_number_input_t *input);

bool SignalKeypadNumberInput_IsActive(
    const signal_keypad_number_input_t *input);
signal_module_status_t SignalKeypadNumberInput_GetModuleStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_KEYPAD_NUMBER_INPUT_H */

