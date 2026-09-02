#ifndef SIGNAL_BUTTON_H
#define SIGNAL_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

#include "signal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The platform callback converts the electrical level to logical pressed. */
typedef signal_result_t (*signal_button_read_fn)(
    void *context, bool *pressed);

typedef struct {
    void *context;
    signal_button_read_fn read_pressed;
    uint8_t debounce_scans;
} signal_button_config_t;

typedef struct {
    bool raw_pressed;
    bool stable_pressed;
    bool pressed;
    bool released;
} signal_button_event_t;

typedef struct {
    signal_button_config_t config;
    bool raw_pressed;
    bool candidate_pressed;
    bool stable_pressed;
    uint8_t candidate_scans;
    bool initialized;
} signal_button_t;
/**
 * @brief 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。
 * @param button `button`（`signal_button_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param config 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalButton_Init(
    signal_button_t *button,
    const signal_button_config_t *config);

/* Call periodically, commonly once every 5 ms. */
signal_result_t SignalButton_Update(
    signal_button_t *button,
    signal_button_event_t *event);
/**
 * @brief 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。
 * @param button `button`（`const signal_button_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param pressed `pressed`（`bool *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalButton_GetPressed(
    const signal_button_t *button,
    bool *pressed);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */

signal_module_status_t SignalButton_GetModuleStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_BUTTON_H */

