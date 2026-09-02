#ifndef SIGNAL_CLIPPING_DETECT_H
#define SIGNAL_CLIPPING_DETECT_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    float low_limit_v;
    float high_limit_v;
} signal_clipping_detect_config_t;

typedef struct
{
    uint32_t low_clipped_count;
    uint32_t high_clipped_count;
    uint32_t clipped_count;
    float clipped_ratio;
    uint8_t is_clipped;
} signal_clipping_detect_result_t;

/**
 * @brief 统计达到低/高限幅阈值的电压样本。
 * @param voltage_v 输入电压数组，单位 V，只读。
 * @param count 样本点数，必须大于 0。
 * @param config 低、高限幅判断阈值，单位 V，且 low_limit_v < high_limit_v。
 * @param result 输出限幅点数、比例和判断标志。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数或数值非法时返回错误码。
 * @note 阈值应略缩进于真实 ADC/前端电源轨，避免只有“精确等于满量程”才报警。
 */
signal_algorithm_status_t SignalClippingDetect_Process(
    const float *voltage_v,
    uint32_t count,
    const signal_clipping_detect_config_t *config,
    signal_clipping_detect_result_t *result);

#endif /* SIGNAL_CLIPPING_DETECT_H */
