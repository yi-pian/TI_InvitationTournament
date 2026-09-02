#ifndef SIGNAL_CONTEST_CONFIG_H
#define SIGNAL_CONTEST_CONFIG_H

/* ===== 你需要根据题目修改；不用的参数可以删除 ===== */
#define SIGNAL_SAMPLE_RATE_HZ        (500000U)
#define SIGNAL_SAMPLE_COUNT          (2048U)
#define SIGNAL_ADC_VREF_V            (3.3f)
#define SIGNAL_EXPECTED_MIN_HZ       (1000.0f)
#define SIGNAL_EXPECTED_MAX_HZ       (100000.0f)
/* AD9833 模块的真实 MCLK。25 MHz 仅是常见模块默认值，必须按模块晶振丝印/实测修改。 */
#define SIGNAL_AD9833_MCLK_HZ        (25000000U)

/* 模拟前端校准系数：ADC 原始码去直流后分别换算为 DUT 端电压和电流。
 * 电流通道若为跨阻放大器，请将 CURRENT_SCALE 改为 1/(跨阻增益 V/A)。 */
#define SIGNAL_VOLTAGE_SCALE_V_PER_CODE \
    (SIGNAL_ADC_VREF_V / 4095.0f)
#define SIGNAL_CURRENT_SCALE_A_PER_CODE \
    (SIGNAL_ADC_VREF_V / 4095.0f)

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
