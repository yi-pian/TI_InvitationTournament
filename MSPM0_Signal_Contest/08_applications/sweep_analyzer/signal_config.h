#ifndef SWEEP_ANALYZER_CONFIG_H
#define SWEEP_ANALYZER_CONFIG_H

#define SIGNAL_SWEEP_START_FREQ_HZ       (1000.0f)
#define SIGNAL_SWEEP_STOP_FREQ_HZ        (10000.0f)
#define SIGNAL_SWEEP_STEP_FREQ_HZ        (1000.0f)
#define SIGNAL_SWEEP_POINT_COUNT         (10U)
#define SIGNAL_SWEEP_SETTLING_TIME_US    (5000U)

#define SIGNAL_SAMPLE_RATE_HZ            (100000U)
#define SIGNAL_SAMPLE_COUNT              (1024U)
#define SIGNAL_ADC_BITS                  (12U)
#define SIGNAL_ADC_VREF_V                (3.3f)
#define SIGNAL_INPUT_SCALE               (1.0f)
#define SIGNAL_INPUT_OFFSET_V            (0.0f)

#define SIGNAL_DAC_UPDATE_RATE_HZ        (100000U)
#define SIGNAL_DAC_BITS                  (12U)
#define SIGNAL_DAC_VREF_V                (3.3f)
#define SIGNAL_DDS_TABLE_COUNT           (256U)
#define SIGNAL_DDS_DMA_BUFFER_COUNT      (1000U)
#define SIGNAL_DDS_AMPLITUDE_PEAK_V      (1.0f)
#define SIGNAL_DDS_OFFSET_V              (1.65f)
#define SIGNAL_DDS_PHASE_DEG             (0.0f)

#define SIGNAL_SWEEP_UART_CSV_ENABLE     (0U)

#if SIGNAL_SWEEP_POINT_COUNT < 2U
#error "Sweep needs at least two points"
#endif
#if SIGNAL_SAMPLE_COUNT > 65535U
#error "ADC DMA sample count exceeds uint16_t API"
#endif
#if SIGNAL_DDS_DMA_BUFFER_COUNT > 65535U
#error "DAC DMA buffer count exceeds adapter API"
#endif

#endif
