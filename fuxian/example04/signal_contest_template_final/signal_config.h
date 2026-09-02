#ifndef SIGNAL_CONTEST_CONFIG_H
#define SIGNAL_CONTEST_CONFIG_H

/* ===== 你需要根据题目修改；不用的参数可以删除 ===== */
#define SIGNAL_SAMPLE_RATE_HZ        (100000U)
#define SIGNAL_SAMPLE_COUNT          (512U)
#define SIGNAL_ADC_VREF_V            (3.3f)
#define SIGNAL_EXPECTED_MIN_HZ       (200.0f)
#define SIGNAL_EXPECTED_MAX_HZ       (20000.0f)
#define SIGNAL_DAC_UPDATE_RATE_HZ    (100000U)
#define SIGNAL_DDS_FREQUENCY_HZ      (1000.0f)

/* example04: one 512-point frame balances FFT resolution and MSPM0 RAM. */
#define SIGNAL_DAC_TABLE_COUNT       (256U)
#define SIGNAL_DAC_OUTPUT_COUNT      (512U)
#define SIGNAL_DAC_BITS              (12U)
#define SIGNAL_ADC_BITS              (12U)
#define SIGNAL_CAPTURE_PRETRIGGER    (128U)
#define SIGNAL_ZERO_EVENT_CAPACITY   (256U)

/* 单次波形页：1 MSPS 下保存 416 us。208 us 触发前历史 +
 * 208 us 触发后数据，因此门限在 50~200 us 任意波的前、中、后部
 * 跨越时，仍有机会同时保留完整波形和两侧基线。 */
#define SIGNAL_SINGLE_CAPTURE_MAX_SAMPLES (416U)
#define SIGNAL_SINGLE_CAPTURE_SLOTS       (3U)
#define SIGNAL_SINGLE_CAPTURE_PRETRIGGER  (208U)
#define SIGNAL_SINGLE_CAPTURE_DMA_BLOCKS  (3U)
#define SIGNAL_SINGLE_CAPTURE_BASELINE_SAMPLES (32U)
#define SIGNAL_SINGLE_CAPTURE_EDGE_MARGIN      (4U)
#define SIGNAL_SINGLE_CAPTURE_MIN_ACTIVITY     (32U)
#define SIGNAL_SINGLE_CAPTURE_QUIET_SAMPLES    (8U)
#define SIGNAL_SINGLE_CAPTURE_ACTIVITY_RUN     (3U)
/* PA17 外部比较器方波至少应具有约 0.2 V 的 ADC 码跨度。 */
#define SIGNAL_CAPTURE_GATE_MIN_SPAN            (256U)

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
