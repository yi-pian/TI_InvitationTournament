#include "signal_reference_backend.h"

#include <math.h>
#include "signal_fft.h"

signal_result_t SignalReference_FFTF32(signal_complex_f32_t *samples,
    size_t count, bool inverse)
{
    return SignalFFT_Execute(samples, count, inverse);
}

float SignalReference_SqrtF32(float value) { return sqrtf(value); }
float SignalReference_SinF32(float radians) { return sinf(radians); }
float SignalReference_CosF32(float radians) { return cosf(radians); }
float SignalReference_Atan2F32(float y, float x) { return atan2f(y, x); }
