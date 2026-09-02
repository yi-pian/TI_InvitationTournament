#ifndef SIGNAL_TRIGGER_CAPTURE_H
#define SIGNAL_TRIGGER_CAPTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

/** @brief 在 raw 数组中要寻找的阈值穿越方向。 */
typedef enum {
    SIGNAL_TRIGGER_RISING = 0,
    SIGNAL_TRIGGER_FALLING,
    SIGNAL_TRIGGER_EITHER
} signal_trigger_edge_t;

/**
 * @brief 软件触发条件，level/hysteresis 的单位均为 ADC 原始码。
 *
 * 上升沿先要求样本进入 level-hysteresis 以下的 armed 状态，之后即使
 * 经过多个采样点，只要到达 level+hysteresis 就触发；下降沿相反。
 * 迟滞为 0 时就是直接跨过 level。
 */
typedef struct {
    uint16_t level;
    uint16_t hysteresis;
    signal_trigger_edge_t edge;
} signal_trigger_config_t;

/**
 * @brief 从已有 raw 帧中寻找满足迟滞条件的第一条边沿。
 * @param search_start 搜索起点之前的样本索引；至少要为边沿比较保留下一点。
 * @param trigger_index 成功时写入边沿的“当前样本”索引。
 * @return OK 找到；NO_DATA 未找到；INVALID_ARGUMENT 为非法数组/范围。
 */
signal_result_t SignalTrigger_Find(const uint16_t *samples, size_t count,
    const signal_trigger_config_t *config, size_t search_start,
    size_t *trigger_index);
/**
 * @brief 将触发点前 pretrigger_count 点开始的连续片段复制到 output。
 * @return OK 成功；INSUFFICIENT_BUFFER 表示所需片段超出原帧。
 */
signal_result_t SignalTrigger_Extract(const uint16_t *samples, size_t count,
    size_t trigger_index, size_t pretrigger_count, uint16_t *output,
    size_t output_count);
/** @brief 返回构建验证证据等级，不表示上游 ADC 已实板验证。 */
signal_module_status_t SignalTrigger_GetModuleStatus(void);

#endif
