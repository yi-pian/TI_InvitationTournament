#include "signal_iqmath_backend.h"

#include <stddef.h>

/* IQmath is deliberately isolated to this translation unit. */
#include <ti/iqmath/include/IQmathLib.h>

signal_iq24_t SignalIQMathQ24_Multiply(signal_iq24_t a, signal_iq24_t b)
{
    return (signal_iq24_t) _IQ24mpy((_iq24) a, (_iq24) b);
}

signal_result_t SignalIQMathQ24_Divide(signal_iq24_t numerator,
    signal_iq24_t denominator, signal_iq24_t *result)
{
    if ((result == NULL) || (denominator == 0)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *result = (signal_iq24_t) _IQ24div((_iq24) numerator,
        (_iq24) denominator);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalIQMathQ24_Sqrt(signal_iq24_t value,
    signal_iq24_t *result)
{
    if ((result == NULL) || (value < 0)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *result = (signal_iq24_t) _IQ24sqrt((_iq24) value);
    return SIGNAL_RESULT_OK;
}

signal_iq24_t SignalIQMathQ24_Sin(signal_iq24_t radians)
{
    return (signal_iq24_t) _IQ24sin((_iq24) radians);
}

signal_iq24_t SignalIQMathQ24_Cos(signal_iq24_t radians)
{
    return (signal_iq24_t) _IQ24cos((_iq24) radians);
}

signal_iq24_t SignalIQMathQ24_Atan2(signal_iq24_t y, signal_iq24_t x)
{
    return (signal_iq24_t) _IQ24atan2((_iq24) y, (_iq24) x);
}
