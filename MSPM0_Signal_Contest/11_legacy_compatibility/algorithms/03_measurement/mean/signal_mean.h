#ifndef SIGNAL_MEAN_H
#define SIGNAL_MEAN_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float mean_value;
} signal_mean_result_t;

/**
 * @brief 计算一组浮点样本的算术平均值。
 * @param samples 输入样本，只读；单位由调用者决定，输出保持相同单位。
 * @param count 样本点数，必须大于 0。
 * @param result 输出平均值。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；空指针、零长度或非有限数返回错误码。
 * @note 使用补偿求和降低长数组累计舍入误差，不修改输入数组。
 */
signal_algorithm_status_t SignalMean_Process(
    const float *samples,
    uint32_t count,
    signal_mean_result_t *result);

#endif /* SIGNAL_MEAN_H */
