#ifndef SPECTRUM_ANALYZER_CONFIG_H
#define SPECTRUM_ANALYZER_CONFIG_H

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
#define SIGNAL_EXPECTED_FREQ_MAX_HZ    (40000U)
#define SIGNAL_SPECTRUM_PEAK_COUNT     (5U)

#if (SIGNAL_SAMPLE_COUNT & (SIGNAL_SAMPLE_COUNT - 1U)) != 0U
#error "SIGNAL_SAMPLE_COUNT must be a power of two"
#endif
#if SIGNAL_SPECTRUM_PEAK_COUNT > 8U
#error "At most eight peaks are exposed"
#endif

#endif
