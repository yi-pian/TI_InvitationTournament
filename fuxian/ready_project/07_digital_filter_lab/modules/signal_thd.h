#ifndef SIGNAL_THD_H
#define SIGNAL_THD_H

#include "signal_algorithm_status.h"
#include "signal_harmonic.h"

typedef struct
{
    float fundamental_energy;
    float harmonic_energy_sum;
    float thd_ratio;
    float thd_percent;
} signal_thd_result_t;

/**
 * @brief 根据 Harmonic 结果计算 sqrt(sum(H2..Hm energy)/H1 energy)。
 * @param harmonics 必须包含 1 阶和至少一个 2 阶以上结果。
 * @param result 输出基波/谐波能量、THD 比值和百分数。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；缺基波或基波能量为零返回错误。
 * @note 同一 FFT/窗/半径下公共标度会在能量比中相消。
 */
signal_algorithm_status_t SignalTHD_Process(
    const signal_harmonic_result_t *harmonics,
    signal_thd_result_t *result);

#endif /* SIGNAL_THD_H */
