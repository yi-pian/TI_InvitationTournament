#ifndef SIGNAL_BLACKMAN_H
#define SIGNAL_BLACKMAN_H

#include <stdint.h>
#include "signal_window.h"

/** @brief 应用对称 Blackman 窗，支持原地，并返回实际窗增益。 */
signal_algorithm_status_t SignalBlackman_Apply(
    const float *input_samples,
    float *output_samples,
    uint32_t count,
    signal_window_result_t *result);

#endif /* SIGNAL_BLACKMAN_H */
