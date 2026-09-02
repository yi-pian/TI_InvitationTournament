#ifndef SIGNAL_CONTEST_CONFIG_H
#define SIGNAL_CONTEST_CONFIG_H

/* ===== 你需要根据题目修改；不用的参数可以删除 ===== */
#define SIGNAL_SAMPLE_RATE_HZ        (100000U)
#define SIGNAL_SAMPLE_COUNT          (1024U)
#define SIGNAL_ADC_VREF_V            (3.3f)
#define SIGNAL_EXPECTED_MIN_HZ       (100.0f)
#define SIGNAL_EXPECTED_MAX_HZ       (20000.0f)
#define SIGNAL_DAC_UPDATE_RATE_HZ    (1000000U)
#define SIGNAL_DDS_FREQUENCY_HZ      (1000.0f)
#define SIGNAL_SWEEP_START_HZ        (500.0f)
#define SIGNAL_SWEEP_STOP_HZ         (8000.0f)
#define SIGNAL_SWEEP_POINTS          (16U)

/* Processing-profile parameters used by signal_pipeline.c. */
#define SIGNAL_ZERO_CROSS_HYSTERESIS (0.005f)
#define SIGNAL_PEAK_COUNT            (5U)
#define SIGNAL_HARMONIC_RADIUS       (1U)
#define SIGNAL_PHASE_FREQUENCY_HZ    (1000.0f)
#define SIGNAL_MAX_PHASE_LAG         (128U)

/* Backward-compatible local name used by the existing template pipeline. */
#define SAMPLE_COUNT SIGNAL_SAMPLE_COUNT

#if SIGNAL_SAMPLE_COUNT == 0U
#error "SIGNAL_SAMPLE_COUNT must be greater than zero"
#endif

#endif /* SIGNAL_CONTEST_CONFIG_H */
