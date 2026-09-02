#ifndef SIGNAL_MATH_H
#define SIGNAL_MATH_H

#include <stddef.h>

#define SIGNAL_PI_F 3.14159265358979323846f
#define SIGNAL_TWO_PI_F (2.0f * SIGNAL_PI_F)

static inline float SignalMath_ClampF32(float value, float low, float high)
{
    return (value < low) ? low : ((value > high) ? high : value);
}

static inline int SignalMath_IsPowerOfTwo(size_t value)
{
    return (value != 0U) && ((value & (value - 1U)) == 0U);
}

#endif /* SIGNAL_MATH_H */
