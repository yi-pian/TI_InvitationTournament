#ifndef SIGNAL_METER_CONFIG_H
#define SIGNAL_METER_CONFIG_H

#define SIGNAL_SAMPLE_RATE_HZ              (100000U)
#define SIGNAL_SAMPLE_COUNT                (1024U)
#define SIGNAL_ADC_CHANNEL_INDEX           (2U)   /* PA25 / ADC0.2 in SysConfig */
#define SIGNAL_ADC_BITS                    (12U)
#define SIGNAL_ADC_VREF_V                  (3.3f)
#define SIGNAL_INPUT_SCALE                 (1.0f)
#define SIGNAL_INPUT_OFFSET_V              (0.0f)
#define SIGNAL_EXPECTED_FREQ_MIN_HZ        (100U)
#define SIGNAL_EXPECTED_FREQ_MAX_HZ        (20000U)
#define SIGNAL_ZERO_CROSS_HYSTERESIS_V     (0.005f)

#define SIGNAL_ENABLE_DC                   (1U)
#define SIGNAL_ENABLE_MIN_MAX              (1U)
#define SIGNAL_ENABLE_VPP                  (1U)
#define SIGNAL_ENABLE_RMS                  (1U)
#define SIGNAL_ENABLE_AC_RMS               (1U)
#define SIGNAL_ENABLE_FREQUENCY            (1U)
#define SIGNAL_RUN_CONTINUOUSLY             (1U)

#if (SIGNAL_SAMPLE_RATE_HZ == 0U)
#error "SIGNAL_SAMPLE_RATE_HZ must be positive"
#endif
#if (SIGNAL_SAMPLE_COUNT < 4U) || (SIGNAL_SAMPLE_COUNT > 65535U)
#error "SIGNAL_SAMPLE_COUNT must be in 4..65535"
#endif
#if (SIGNAL_ADC_BITS == 0U) || (SIGNAL_ADC_BITS > 16U)
#error "SIGNAL_ADC_BITS must be in 1..16"
#endif
#if (SIGNAL_ENABLE_FREQUENCY != 0U) && \
    (SIGNAL_SAMPLE_RATE_HZ < (2U * SIGNAL_EXPECTED_FREQ_MAX_HZ))
#error "Sampling rate violates Nyquist for the configured frequency range"
#endif

#endif /* SIGNAL_METER_CONFIG_H */
