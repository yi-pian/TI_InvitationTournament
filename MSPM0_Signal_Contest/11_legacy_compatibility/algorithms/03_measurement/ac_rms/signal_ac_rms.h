#ifndef SIGNAL_AC_RMS_H
#define SIGNAL_AC_RMS_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float mean_voltage_v;
    float ac_rms_v;
} signal_ac_rms_result_t;

/**
 * @brief 计算去除平均直流分量后的交流有效值。
 * @param voltage_v 输入电压数组，单位 V，只读。
 * @param count 样本点数，必须大于 0。
 * @param result 输出平均电压和交流 RMS，单位 V。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法时返回错误码。
 * @note 采用两遍扫描，不修改输入；记录长度最好覆盖整数个或多个完整周期。
 */
signal_algorithm_status_t SignalACRMS_Process(
    const float *voltage_v,
    uint32_t count,
    signal_ac_rms_result_t *result);

#endif /* SIGNAL_AC_RMS_H */
