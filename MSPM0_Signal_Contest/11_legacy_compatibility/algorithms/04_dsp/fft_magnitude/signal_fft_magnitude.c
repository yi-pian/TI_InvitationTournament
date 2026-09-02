#include "signal_fft_magnitude.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalFFTMagnitude_Process(
    const signal_complex_f32_t *spectrum,
    uint32_t fft_size,
    float *magnitude,
    uint32_t magnitude_capacity,
    signal_fft_magnitude_result_t *result)
{
    uint32_t bin;
    uint32_t bin_count;

    if ((spectrum == NULL) || (magnitude == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((fft_size < 2U) || ((fft_size & 1U) != 0U))
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    bin_count = (fft_size / 2U) + 1U;
    if (magnitude_capacity < bin_count)
    {
        return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
    }

    for (bin = 0U; bin < bin_count; ++bin)
    {
        float real = spectrum[bin].real;
        float imag = spectrum[bin].imag;
        if (!isfinite(real) || !isfinite(imag))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        magnitude[bin] = hypotf(real, imag);
        if (!isfinite(magnitude[bin]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }
    result->bin_count = bin_count;
    return SIGNAL_ALGORITHM_OK;
}
