#ifndef SIGNAL_STATISTICS_H
#define SIGNAL_STATISTICS_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    uint32_t count;
    float mean_value;
    float min_value;
    float max_value;
    float population_variance;
    float population_stddev;
    float sample_variance;
    float sample_stddev;
    uint8_t sample_variance_valid;
} signal_statistics_result_t;

/**
 * @brief 一次扫描计算数量、均值、极值、总体/样本方差和标准差。
 * @param samples 输入浮点样本，只读；均值/极值/标准差保持输入单位，方差为输入单位的平方。
 * @param count 样本点数，必须大于 0。
 * @param result 输出统计量；count<2 时样本方差字段为 0 且 valid=0。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法时返回错误码。
 * @note 使用 Welford 更新，适合避免“大直流 + 小波动”时的严重相消误差。
 */
signal_algorithm_status_t SignalStatistics_Process(
    const float *samples,
    uint32_t count,
    signal_statistics_result_t *result);

#endif /* SIGNAL_STATISTICS_H */
