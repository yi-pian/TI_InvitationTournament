#ifndef SIGNAL_MATH_BACKEND_H
#define SIGNAL_MATH_BACKEND_H

#include <math.h>

#include "signal_math_backend_config.h"

#if SIGNAL_MATH_BACKEND != SIGNAL_MATH_BACKEND_REFERENCE_FLOAT
#include <ti/iqmath/include/IQmathLib.h>
#endif

/*
 * 内部标量数学 Adapter。它不是面向 Recipe 的公开积木；Recipe 仍写 Phase/RMS。
 * RTS 与 MATHACL 使用相同 IQMath C API，真正后端由工程链接的 iqmath.a 决定。
 */
static inline float SignalMathBackend_SqrtF(float value)
{
#if SIGNAL_MATH_BACKEND == SIGNAL_MATH_BACKEND_REFERENCE_FLOAT
    return sqrtf(value);
#else
    float scaled = value;
    float restore = 1.0f;

    if (value <= 0.0f)
    {
        return (value == 0.0f) ? 0.0f : NAN;
    }
    /* IQ24 正数上限约 128；缩到 [0.25,64] 后再开方，避免溢出和小数被量化为 0。 */
    while (scaled > 64.0f) {
        scaled *= 0.25f;
        restore *= 2.0f;
    }
    while (scaled < 0.25f) {
        scaled *= 4.0f;
        restore *= 0.5f;
    }
    return _IQ24toF(_IQ24sqrt(_IQ24(scaled))) * restore;
#endif
}

static inline float SignalMathBackend_Atan2F(float y, float x)
{
#if SIGNAL_MATH_BACKEND == SIGNAL_MATH_BACKEND_REFERENCE_FLOAT
    return atan2f(y, x);
#else
    float absolute_x = fabsf(x);
    float absolute_y = fabsf(y);
    float scale = (absolute_x > absolute_y) ? absolute_x : absolute_y;

    if (scale == 0.0f)
    {
        return 0.0f;
    }
    /* 归一化到 [-1,1]，避免任意物理幅值超出 IQ24 输入范围。 */
    return _IQ24toF(_IQ24atan2(_IQ24(y / scale), _IQ24(x / scale)));
#endif
}

#endif /* SIGNAL_MATH_BACKEND_H */
