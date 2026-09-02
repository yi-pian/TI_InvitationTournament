#include "signal_rectangular.h"

signal_algorithm_status_t SignalRectangular_Apply(
    const float *input_samples,
    float *output_samples,
    uint32_t count,
    signal_window_result_t *result)
{
    return SignalWindow_Apply(input_samples, output_samples, count,
                              SIGNAL_WINDOW_RECTANGULAR, result);
}
