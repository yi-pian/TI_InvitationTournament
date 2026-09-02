#ifndef SIGNAL_MINMAX_H
#define SIGNAL_MINMAX_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float min_value;
    float max_value;
    uint32_t min_index;
    uint32_t max_index;
} signal_minmax_result_t;

/**
 * @brief 查找浮点样本中的最小值、最大值及首次出现的位置。
 * @param samples 输入样本，只读；结果与输入使用相同单位。
 * @param count 样本点数，必须大于 0。
 * @param result 输出最小值、最大值和索引。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法时返回错误码。
 * @note 对毛刺非常敏感；有异常点时应改用 RobustPeakToPeak 等鲁棒算法。
 */
signal_algorithm_status_t SignalMinMax_Process(
    const float *samples,
    uint32_t count,
    signal_minmax_result_t *result);

#endif /* SIGNAL_MINMAX_H */
