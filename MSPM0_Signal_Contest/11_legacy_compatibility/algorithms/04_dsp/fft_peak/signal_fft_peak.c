#include "signal_fft_peak.h"

#include <math.h>
#include <stddef.h>

#include "signal_peak_detect.h"

signal_algorithm_status_t SignalFFTPeak_Process(
    const float *magnitude,
    uint32_t magnitude_count,
    uint32_t first_bin,
    uint32_t last_bin,
    float sample_rate_hz,
    uint32_t fft_size,
    signal_fft_peak_result_t *result)
{
    signal_fft_peak_result_t temporary;
    signal_peak_detect_result_t peak;
    signal_algorithm_status_t status;

    if (result == NULL)
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (!isfinite(sample_rate_hz) || (sample_rate_hz <= 0.0f) ||
        (fft_size < 2U))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    status = SignalPeakDetect_Process(magnitude, magnitude_count, first_bin,
                                     last_bin, &peak);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    temporary.bin = peak.peak_index;
    temporary.peak_value = peak.peak_value;
    temporary.frequency_hz = ((float)peak.peak_index * sample_rate_hz) /
                             (float)fft_size;
    *result = temporary;
    return SIGNAL_ALGORITHM_OK;
}
