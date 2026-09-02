#ifndef DDS_GENERATOR_CONFIG_H
#define DDS_GENERATOR_CONFIG_H

#define SIGNAL_DDS_FREQUENCY_HZ        (1000.0f)
#define SIGNAL_DDS_AMPLITUDE_PEAK_V    (1.0f)
#define SIGNAL_DDS_OFFSET_V            (1.65f)
#define SIGNAL_DDS_PHASE_DEG           (0.0f)
#define SIGNAL_DAC_UPDATE_RATE_HZ      (100000U)
#define SIGNAL_DAC_VREF_V              (3.3f)
#define SIGNAL_DAC_BITS                (12U)
#define SIGNAL_DDS_TABLE_COUNT         (256U)
#define SIGNAL_DDS_DMA_BUFFER_COUNT    (1000U)
#define SIGNAL_DAC_OUTPUT_PIN          (15U) /* PA15 / DAC_OUT */

#if SIGNAL_DDS_TABLE_COUNT < 2U
#error "DDS lookup table needs at least two points"
#endif
#if SIGNAL_DDS_DMA_BUFFER_COUNT > 65535U
#error "DMA buffer count exceeds adapter limit"
#endif

#endif
