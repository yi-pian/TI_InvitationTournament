#ifndef SIGNAL_CONTEST_CONFIG_H
#define SIGNAL_CONTEST_CONFIG_H

/* ===== 本题需要的采样参数；现场改题时只改这里 ===== */
#define SIGNAL_SAMPLE_RATE_HZ        (500000U)
#define SIGNAL_SAMPLE_COUNT          (512U)
#define SIGNAL_ADC_VREF_V            (3.3f)

#if SIGNAL_SAMPLE_COUNT == 0U
#error "SIGNAL_SAMPLE_COUNT must be greater than zero"
#endif

#endif /* SIGNAL_CONTEST_CONFIG_H */
