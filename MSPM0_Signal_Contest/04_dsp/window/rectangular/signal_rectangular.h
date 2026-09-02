#ifndef SIGNAL_RECTANGULAR_H
#define SIGNAL_RECTANGULAR_H

#include <stdint.h>
#include "signal_window.h"

/** @brief 应用矩形窗（系数全 1），支持原地，输入输出单位相同。 */
signal_algorithm_status_t SignalRectangular_Apply(
    const float *input_samples,
    float *output_samples,
    uint32_t count,
    signal_window_result_t *result);

#endif /* SIGNAL_RECTANGULAR_H */
