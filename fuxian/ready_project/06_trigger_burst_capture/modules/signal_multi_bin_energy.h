#ifndef SIGNAL_MULTI_BIN_ENERGY_H
#define SIGNAL_MULTI_BIN_ENERGY_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    uint32_t start_bin;
    uint32_t end_bin;
    float energy;
    float root_sum_square;
} signal_multi_bin_energy_result_t;

/**
 * @brief 把中心 bin 左右指定半径内的 magnitude 平方后求和。
 * @param magnitude 非负线性 magnitude 数组。
 * @param bin_count 数组长度。
 * @param center_bin 中心索引。
 * @param radius_bins 左右半径；0 为 BASIC 单 bin。
 * @param result 输出实际起止 bin、平方和与平方根。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；范围或数值非法返回错误码。
 * @note 边界处窗口会裁剪；同一窗/FFT 标度下，能量比可用于 THD。
 */
signal_algorithm_status_t SignalMultiBinEnergy_Process(
    const float *magnitude,
    uint32_t bin_count,
    uint32_t center_bin,
    uint32_t radius_bins,
    signal_multi_bin_energy_result_t *result);

#endif /* SIGNAL_MULTI_BIN_ENERGY_H */
