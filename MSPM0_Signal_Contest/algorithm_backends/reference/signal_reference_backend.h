#ifndef SIGNAL_REFERENCE_BACKEND_H
#define SIGNAL_REFERENCE_BACKEND_H

#include <stdbool.h>
#include <stddef.h>
#include "signal_status.h"
#include "signal_types.h"

signal_result_t SignalReference_FFTF32(signal_complex_f32_t *samples,
    size_t count, bool inverse);
float SignalReference_SqrtF32(float value);
float SignalReference_SinF32(float radians);
float SignalReference_CosF32(float radians);
float SignalReference_Atan2F32(float y, float x);

#endif
