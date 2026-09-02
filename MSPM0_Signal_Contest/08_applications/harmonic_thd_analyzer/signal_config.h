#ifndef HARMONIC_THD_CONFIG_H
#define HARMONIC_THD_CONFIG_H

#define SIGNAL_SAMPLE_RATE_HZ          (100000U)
#ifndef SIGNAL_SAMPLE_COUNT
#define SIGNAL_SAMPLE_COUNT            (1024U)
#endif
#define SIGNAL_ADC_CHANNEL_INDEX       (2U)
#define SIGNAL_ADC_BITS                (12U)
#define SIGNAL_ADC_VREF_V              (3.3f)
#define SIGNAL_INPUT_SCALE             (1.0f)
#define SIGNAL_INPUT_OFFSET_V          (0.0f)
#define SIGNAL_EXPECTED_FREQ_MIN_HZ    (100U)
#define SIGNAL_EXPECTED_FREQ_MAX_HZ    (9000U) /* H5 must remain below Nyquist */
#define SIGNAL_HARMONIC_BIN_RADIUS     (2U)

#if (SIGNAL_SAMPLE_COUNT & (SIGNAL_SAMPLE_COUNT - 1U)) != 0U
#error "SIGNAL_SAMPLE_COUNT must be a power of two"
#endif
#if (5U * SIGNAL_EXPECTED_FREQ_MAX_HZ) >= (SIGNAL_SAMPLE_RATE_HZ / 2U)
#error "H5 exceeds or reaches Nyquist"
#endif

#endif
