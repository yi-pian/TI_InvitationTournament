#ifndef SIGNAL_ANALYZER_CONFIG_H
#define SIGNAL_ANALYZER_CONFIG_H

#include "signal_features.h"

#define SIGNAL_SAMPLE_RATE_HZ              (100000U)
#ifndef SIGNAL_SAMPLE_COUNT
#define SIGNAL_SAMPLE_COUNT                (512U)
#endif
#define SIGNAL_ADC_BITS                    (12U)
#define SIGNAL_ADC_A_VREF_V                (3.3f)
#define SIGNAL_ADC_B_VREF_V                (3.3f)
#define SIGNAL_INPUT_A_SCALE               (1.0f)
#define SIGNAL_INPUT_B_SCALE               (1.0f)
#define SIGNAL_INPUT_A_OFFSET_V            (0.0f)
#define SIGNAL_INPUT_B_OFFSET_V            (0.0f)
#define SIGNAL_EXPECTED_FREQ_MIN_HZ        (100.0f)
#define SIGNAL_EXPECTED_FREQ_MAX_HZ        (20000.0f)
#define SIGNAL_ZERO_CROSS_HYSTERESIS_V     (0.005f)
#define SIGNAL_HARMONIC_BIN_RADIUS         (2U)
#define SIGNAL_SPECTRAL_BAND_RADIUS        (2U)
#define SIGNAL_KNOWN_PHASE_FREQUENCY_HZ    (1000.0f)
#define SIGNAL_MAX_CORRELATION_LAG         (64U)
#define SIGNAL_SPECTRUM_PEAK_COUNT         (5U)

#if (SIGNAL_SAMPLE_COUNT & (SIGNAL_SAMPLE_COUNT - 1U)) != 0U
#error "FFT-capable analyzer requires power-of-two N"
#endif
#if SIGNAL_MAX_CORRELATION_LAG >= SIGNAL_SAMPLE_COUNT
#error "Correlation lag must be smaller than N"
#endif

#endif
