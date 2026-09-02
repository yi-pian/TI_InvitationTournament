#ifndef SIGNAL_RMS_H
#define SIGNAL_RMS_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float rms_v;
} signal_rms_result_t;

/**
 * @brief 计算电压样本的总有效值 RMS，包含直流分量。
 * @param voltage_v 输入电压数组，单位 V，只读。
 * @param count 样本点数，必须大于 0。
 * @param result 输出总 RMS，单位 V。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法时返回错误码。
 * @note 若只想测交流有效值，应先 RemoveDC，或直接使用 SignalACRMS_Process。
 */
signal_algorithm_status_t SignalRMS_Process(
    const float *voltage_v,
    uint32_t count,
    signal_rms_result_t *result);

#endif /* SIGNAL_RMS_H */
