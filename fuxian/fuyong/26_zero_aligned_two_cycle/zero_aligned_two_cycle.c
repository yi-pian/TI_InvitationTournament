/* fuyong REUSE: 26_zero_aligned_two_cycle
 *
 * 来源：
 *   - 11_zero_cross_frequency：过零检测和亚采样插值；
 *   - 23_trigger_capture：先确定触发点，再确定要显示的连续窗口。
 *
 * 此文件可直接复制到需要“固定显示两个周期”的 main.c 中。
 * 依赖 11_zero_cross_frequency/modules 下的两个过零模块。
 */
#include <stdbool.h>
#include <stdint.h>

#include "signal_zero_cross.h"
#include "signal_zero_cross_interpolation.h"

/* ===== 要显示几个完整周期，只改这一行 =====
 * 2U：显示 2 个周期；3U：显示 3 个周期；4U：显示 4 个周期……
 * 原理：显示 N 个周期，需要 N+1 个同方向上升过零点。
 *       第 1 个上升过零点为左端点，第 N+1 个为右端点。
 * 注意：ADC 一帧数据中必须实际包含这 N+1 个上升过零点，否则函数返回 false。
 */
#define ZERO_ALIGNED_CYCLE_COUNT (2U)

typedef struct
{
    float start_sample; /* 第一个上升过零点的浮点采样位置 */
    float end_sample;   /* 两个周期后的上升过零点的浮点采样位置 */
} zero_aligned_two_cycle_window_t;

/* 找到一个“从上升过零到 ZERO_ALIGNED_CYCLE_COUNT 个周期后上升过零”的窗口。
 * 绘图端必须对 start_sample/end_sample 做线性插值，才能使屏幕左右边界
 * 真正显示为 0 V；直接转换为整数下标会丢失这个性质。
 */
static bool ZeroAlignedTwoCycle_Find(
    const float *centered_samples,
    uint32_t sample_count,
    signal_zero_cross_event_t *events,
    uint32_t event_capacity,
    float *crossing_positions,
    zero_aligned_two_cycle_window_t *window)
{
    const signal_zero_cross_config_t zero_config = {
        0.0f, 0.005f, SIGNAL_ZERO_CROSS_RISING
    };
    signal_zero_cross_result_t zero_result;
    signal_zero_cross_interpolation_result_t interpolation_result;
    uint32_t index;

    if (SignalZeroCross_Process(centered_samples, sample_count,
            &zero_config, events, event_capacity, &zero_result) !=
            SIGNAL_ALGORITHM_OK ||
        zero_result.rising_count < (ZERO_ALIGNED_CYCLE_COUNT + 1U)) {
        return false;
    }

    if (SignalZeroCrossInterpolation_Process(centered_samples, sample_count,
            0.0f, events, zero_result.event_count, crossing_positions,
            event_capacity, &interpolation_result) != SIGNAL_ALGORITHM_OK ||
        interpolation_result.position_count <
            (ZERO_ALIGNED_CYCLE_COUNT + 1U)) {
        return false;
    }

    for (index = 0U;
         index + ZERO_ALIGNED_CYCLE_COUNT < interpolation_result.position_count;
         ++index) {
        if (crossing_positions[index + ZERO_ALIGNED_CYCLE_COUNT] >
            crossing_positions[index]) {
            window->start_sample = crossing_positions[index];
            window->end_sample =
                crossing_positions[index + ZERO_ALIGNED_CYCLE_COUNT];
            return true;
        }
    }
    return false;
}
