#ifndef SIGNAL_ROTARY_ENCODER_H
#define SIGNAL_ROTARY_ENCODER_H

#include <stdbool.h>
#include <stdint.h>

#include "signal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The callback returns the electrical high/low level of one GPIO input. */
typedef signal_result_t (*signal_rotary_encoder_read_level_fn)(
    void *context, bool *high);

typedef struct {
    void *context;
    signal_rotary_encoder_read_level_fn read_a_level;
    signal_rotary_encoder_read_level_fn read_b_level;
    /* Optional. Leave NULL when the encoder has no push button. */
    signal_rotary_encoder_read_level_fn read_button_level;
    /* Mechanical encoders commonly produce 1, 2, or 4 valid transitions/detent. */
    uint8_t transitions_per_step;
    uint8_t button_debounce_scans;
    bool button_active_low;
} signal_rotary_encoder_config_t;

typedef struct {
    int8_t step_delta;       /* -1, 0, or +1 for this update. */
    int32_t position;        /* Saturating accumulated step count. */
    bool invalid_transition; /* Both A and B changed between two scans. */
    bool raw_button_pressed;
    bool stable_button_pressed;
    bool button_pressed;
    bool button_released;
} signal_rotary_encoder_event_t;

typedef struct {
    signal_rotary_encoder_config_t config;
    uint8_t previous_ab;
    int8_t transition_accumulator;
    int32_t position;
    uint32_t invalid_transition_count;
    bool button_candidate_pressed;
    bool button_stable_pressed;
    uint8_t button_candidate_scans;
    bool initialized;
} signal_rotary_encoder_t;
/**
 * @brief 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。
 * @param encoder `encoder`（`signal_rotary_encoder_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param config 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalRotaryEncoder_Init(
    signal_rotary_encoder_t *encoder,
    const signal_rotary_encoder_config_t *config);

/* Call periodically. Start with polling; move GPIO reads to IRQ only if needed. */
signal_result_t SignalRotaryEncoder_Update(
    signal_rotary_encoder_t *encoder,
    signal_rotary_encoder_event_t *event);
/**
 * @brief 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。
 * @param encoder `encoder`（`const signal_rotary_encoder_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param position `position`（`int32_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalRotaryEncoder_GetPosition(
    const signal_rotary_encoder_t *encoder,
    int32_t *position);
/**
 * @brief 修改模块的一个运行参数；若模块有 BUSY/RUNNING 状态，应在空闲时修改。
 * @param encoder `encoder`（`signal_rotary_encoder_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param position `position`（`int32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalRotaryEncoder_SetPosition(
    signal_rotary_encoder_t *encoder,
    int32_t position);
/**
 * @brief 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。
 * @param encoder `encoder`（`const signal_rotary_encoder_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param count 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalRotaryEncoder_GetInvalidTransitionCount(
    const signal_rotary_encoder_t *encoder,
    uint32_t *count);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */

signal_module_status_t SignalRotaryEncoder_GetModuleStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_ROTARY_ENCODER_H */

