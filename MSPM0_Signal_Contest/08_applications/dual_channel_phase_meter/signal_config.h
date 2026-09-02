#ifndef DUAL_PHASE_CONFIG_H
#define DUAL_PHASE_CONFIG_H

#define SIGNAL_SAMPLE_RATE_HZ          (100000U)
#ifndef SIGNAL_SAMPLE_COUNT
#define SIGNAL_SAMPLE_COUNT            (512U)
#endif
#define SIGNAL_ADC_A_CHANNEL_INDEX     (2U)  /* PA25 / ADC0.2 */
#define SIGNAL_ADC_B_CHANNEL_INDEX     (2U)  /* PA17 / ADC1.2 */
#define SIGNAL_ADC_BITS                (12U)
#define SIGNAL_ADC_A_VREF_V            (3.3f)
#define SIGNAL_ADC_B_VREF_V            (3.3f)
#define SIGNAL_INPUT_A_SCALE           (1.0f)
#define SIGNAL_INPUT_B_SCALE           (1.0f)
#define SIGNAL_INPUT_A_OFFSET_V        (0.0f)
#define SIGNAL_INPUT_B_OFFSET_V        (0.0f)
#define SIGNAL_KNOWN_FREQUENCY_HZ      (1000.0f)
#define SIGNAL_MAX_CORRELATION_LAG     (64U)

#if (SIGNAL_SAMPLE_COUNT & (SIGNAL_SAMPLE_COUNT - 1U)) != 0U
#error "SIGNAL_SAMPLE_COUNT must be a power of two"
#endif
#if SIGNAL_MAX_CORRELATION_LAG >= SIGNAL_SAMPLE_COUNT
#error "Maximum lag must be smaller than N"
#endif

#endif
