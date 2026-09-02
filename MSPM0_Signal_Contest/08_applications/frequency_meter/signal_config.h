#ifndef FREQUENCY_METER_CONFIG_H
#define FREQUENCY_METER_CONFIG_H

#define SIGNAL_FREQUENCY_METHOD_CAPTURE      (1U)
#define SIGNAL_FREQUENCY_METHOD_ZERO_CROSS   (2U)
#define SIGNAL_FREQUENCY_METHOD_FFT          (3U)

#ifndef SIGNAL_FREQUENCY_METHOD
#define SIGNAL_FREQUENCY_METHOD              SIGNAL_FREQUENCY_METHOD_ZERO_CROSS
#endif

#define SIGNAL_SAMPLE_RATE_HZ                (100000U)
#ifndef SIGNAL_SAMPLE_COUNT
#define SIGNAL_SAMPLE_COUNT                  (1024U)
#endif
#define SIGNAL_ADC_CHANNEL_INDEX             (2U)
#define SIGNAL_ADC_BITS                      (12U)
#define SIGNAL_ADC_VREF_V                    (3.3f)
#define SIGNAL_INPUT_SCALE                   (1.0f)
#define SIGNAL_INPUT_OFFSET_V                (0.0f)
#define SIGNAL_EXPECTED_FREQ_MIN_HZ          (100U)
#define SIGNAL_EXPECTED_FREQ_MAX_HZ          (20000U)
#define SIGNAL_ZERO_CROSS_HYSTERESIS_V       (0.005f)

#define SIGNAL_CAPTURE_CLOCK_HZ              (32000000U)
#define SIGNAL_CAPTURE_COUNTER_MODULUS       (SIGNAL_CAPTURE_INST_LOAD_VALUE + 1U)
#define SIGNAL_CAPTURE_TIMESTAMP_COUNT       (8U)
#define SIGNAL_CAPTURE_TIMEOUT_OVERFLOWS     (500U)

#if (SIGNAL_FREQUENCY_METHOD < 1U) || (SIGNAL_FREQUENCY_METHOD > 3U)
#error "Choose capture(1), zero-cross(2), or FFT(3)"
#endif
#if (SIGNAL_SAMPLE_COUNT & (SIGNAL_SAMPLE_COUNT - 1U)) != 0U
#error "FFT-capable build requires a power-of-two SIGNAL_SAMPLE_COUNT"
#endif

#endif /* FREQUENCY_METER_CONFIG_H */
