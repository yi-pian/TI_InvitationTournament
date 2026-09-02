#ifndef SIGNAL_LATCHING_BUTTON_SWITCH_H
#define SIGNAL_LATCHING_BUTTON_SWITCH_H

#include <stdbool.h>
#include <stdint.h>

#include "signal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The platform callback converts the electrical level to logical switch ON. */
typedef signal_result_t (*signal_latching_button_switch_read_fn)(
    void *context, bool *on);

typedef struct {
    void *context;
    signal_latching_button_switch_read_fn read_on;
    uint8_t debounce_scans;
} signal_latching_button_switch_config_t;

typedef struct {
    bool raw_on;
    bool state_valid;
    bool stable_on;
    bool changed;
    bool turned_on;
    bool turned_off;
} signal_latching_button_switch_event_t;

typedef struct {
    signal_latching_button_switch_config_t config;
    bool raw_on;
    bool candidate_on;
    bool stable_on;
    uint8_t candidate_scans;
    bool state_valid;
    bool initialized;
} signal_latching_button_switch_t;
/**
 * @brief 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。
 * @param switch_module `switch_module`（`signal_latching_button_switch_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param config 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalLatchingButtonSwitch_Init(
    signal_latching_button_switch_t *switch_module,
    const signal_latching_button_switch_config_t *config);

/* Call periodically, commonly once every 5 ms. */
signal_result_t SignalLatchingButtonSwitch_Update(
    signal_latching_button_switch_t *switch_module,
    signal_latching_button_switch_event_t *event);
/**
 * @brief 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。
 * @param switch_module `switch_module`（`const signal_latching_button_switch_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param on `on`（`bool *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalLatchingButtonSwitch_GetState(
    const signal_latching_button_switch_t *switch_module,
    bool *on);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */

signal_module_status_t SignalLatchingButtonSwitch_GetModuleStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_LATCHING_BUTTON_SWITCH_H */

