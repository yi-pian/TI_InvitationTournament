#ifndef SIGNAL_TYPES_H
#define SIGNAL_TYPES_H

#include <stddef.h>
#include <stdint.h>

/** 只读 ADC 原始帧；采样率是配置/测得值，由生产者说明来源。 */
typedef struct {
    const uint16_t *data;
    size_t count;
    uint32_t sample_rate_hz;
    uint8_t adc_bits;
    float reference_voltage_v;
} signal_u16_frame_t;

/** 可写 ADC 原始帧。 */
typedef struct {
    uint16_t *data;
    size_t capacity;
    size_t count;
    uint32_t sample_rate_hz;
} signal_u16_buffer_t;

/** 只读单精度帧。 */
typedef struct {
    const float *data;
    size_t count;
    float sample_rate_hz;
} signal_f32_frame_t;

/** 可写单精度帧。 */
typedef struct {
    float *data;
    size_t capacity;
    size_t count;
    float sample_rate_hz;
} signal_f32_buffer_t;

typedef struct {
    float real;
    float imag;
} signal_complex_f32_t;

#endif /* SIGNAL_TYPES_H */
