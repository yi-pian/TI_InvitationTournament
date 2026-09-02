#ifndef SIGNAL_IQMATH_BACKEND_H
#define SIGNAL_IQMATH_BACKEND_H

#include <stdint.h>
#include "signal_status.h"

/*
 * Public wrapper type is plain signed Q24. IQmath symbols stay private to the
 * implementation. Values use 24 fractional bits. Angles accepted/returned
 * by the trigonometric functions are radians in Q24. The linked iqmath.a
 * selects RTS or MathACL behavior without changing this public interface.
 */
typedef int32_t signal_iq24_t;

signal_iq24_t SignalIQMathQ24_Multiply(signal_iq24_t a, signal_iq24_t b);
signal_result_t SignalIQMathQ24_Divide(signal_iq24_t numerator,
    signal_iq24_t denominator, signal_iq24_t *result);
signal_result_t SignalIQMathQ24_Sqrt(signal_iq24_t value,
    signal_iq24_t *result);
signal_iq24_t SignalIQMathQ24_Sin(signal_iq24_t radians);
signal_iq24_t SignalIQMathQ24_Cos(signal_iq24_t radians);
signal_iq24_t SignalIQMathQ24_Atan2(signal_iq24_t y, signal_iq24_t x);

#endif
