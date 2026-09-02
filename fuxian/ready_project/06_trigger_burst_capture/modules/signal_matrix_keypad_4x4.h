#ifndef SIGNAL_MATRIX_KEYPAD_4X4_H
#define SIGNAL_MATRIX_KEYPAD_4X4_H

#include <stdbool.h>
#include <stdint.h>

#include "signal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SIGNAL_MATRIX_KEYPAD_4X4_ROWS       4U
#define SIGNAL_MATRIX_KEYPAD_4X4_COLUMNS    4U
#define SIGNAL_MATRIX_KEYPAD_4X4_KEY_COUNT 16U

/*
 * drive_row(active=true) selects one row. A usual MSPM0 adapter implements
 * this by driving that row low; active=false drives it high.
 * read_column returns active=true when the selected row and column are joined
 * by a pressed key. A usual pull-up input therefore maps a low pin to true.
 */
typedef signal_result_t (*signal_matrix_keypad_drive_row_fn)(
    void *context, uint8_t row, bool active);
typedef signal_result_t (*signal_matrix_keypad_read_column_fn)(
    void *context, uint8_t column, bool *active);
typedef void (*signal_matrix_keypad_delay_us_fn)(
    void *context, uint32_t microseconds);

typedef struct {
    void *context;
    signal_matrix_keypad_drive_row_fn drive_row;
    signal_matrix_keypad_read_column_fn read_column;
    signal_matrix_keypad_delay_us_fn delay_us; /* Optional if settle_us is 0. */
    uint32_t settle_us;
    uint8_t debounce_scans; /* Same raw mask required this many scans. */
    const char *keymap;      /* Optional 16-byte row-major map. */
} signal_matrix_keypad_4x4_config_t;

typedef struct {
    uint16_t raw_mask;
    uint16_t stable_mask;
    uint16_t pressed_mask;
    uint16_t released_mask;
    bool ghost_possible;
} signal_matrix_keypad_4x4_event_t;

typedef struct {
    signal_matrix_keypad_4x4_config_t config;
    uint16_t raw_mask;
    uint16_t candidate_mask;
    uint16_t stable_mask;
    uint8_t candidate_scans;
    bool initialized;
} signal_matrix_keypad_4x4_t;
/**
 * @brief 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。
 * @param keypad `keypad`（`signal_matrix_keypad_4x4_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param config 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalMatrixKeypad4x4_Init(
    signal_matrix_keypad_4x4_t *keypad,
    const signal_matrix_keypad_4x4_config_t *config);

/* Call periodically, commonly every 5 ms. One call scans all 16 positions. */
signal_result_t SignalMatrixKeypad4x4_Scan(
    signal_matrix_keypad_4x4_t *keypad,
    signal_matrix_keypad_4x4_event_t *event);

/*
 * MSPM0G3507 fixed-pin convenience API. Call after SYSCFG_DL_init(), commonly
 * every 5 ms. It owns its internal keypad object, filters possible ghost keys,
 * and writes a newly debounced key symbol on SIGNAL_RESULT_OK. It returns
 * SIGNAL_RESULT_NO_DATA when there is no new trusted key.
 */
#if defined(__MSPM0G3507__)
signal_result_t SignalMatrixKeypad4x4_ReadNewSymbol(char *symbol);
#endif
/**
 * @brief 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。
 * @param keypad `keypad`（`const signal_matrix_keypad_4x4_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param key_index 索引或通道号；范围由相应数组长度、FFT bin 数或当前硬件配置决定。
 * @param symbol `symbol`（`char *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalMatrixKeypad4x4_GetKey(
    const signal_matrix_keypad_4x4_t *keypad,
    uint8_t key_index,
    char *symbol);
/**
 * @brief 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。
 * @param keypad `keypad`（`const signal_matrix_keypad_4x4_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param symbol `symbol`（`char *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param key_index 索引或通道号；范围由相应数组长度、FFT bin 数或当前硬件配置决定。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalMatrixKeypad4x4_GetFirstPressed(
    const signal_matrix_keypad_4x4_t *keypad,
    char *symbol,
    uint8_t *key_index);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */

signal_module_status_t SignalMatrixKeypad4x4_GetModuleStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_MATRIX_KEYPAD_4X4_H */

