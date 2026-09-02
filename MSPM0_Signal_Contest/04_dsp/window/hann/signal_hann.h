#ifndef SIGNAL_HANN_H
#define SIGNAL_HANN_H

#include <stdint.h>
#include "signal_window.h"

/** @brief 应用对称 Hann 窗，支持原地，并返回本次点数对应的实际窗增益。 */
signal_algorithm_status_t SignalHann_Apply(
    const float *input_samples,
    float *output_samples,
    uint32_t count,
    signal_window_result_t *result);

#endif /* SIGNAL_HANN_H */
