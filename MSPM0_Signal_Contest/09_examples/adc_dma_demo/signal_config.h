#ifndef SIGNAL_CONFIG_H
#define SIGNAL_CONFIG_H

/** 目标 Timer 事件触发率，单位 Hz；可由编译器 -D 覆盖。 */
#ifndef SIGNAL_SAMPLE_RATE_HZ
#define SIGNAL_SAMPLE_RATE_HZ (100000U)
#endif

/** 单帧采样点数；可由编译器 -D 覆盖，不需要修改 signal_adc_dma.c。 */
#ifndef SIGNAL_SAMPLE_COUNT
#define SIGNAL_SAMPLE_COUNT (1024U)
#endif

/** Start -> Done 连续验收帧数；本阶段默认执行 100 次。 */
#ifndef SIGNAL_ACCEPTANCE_BLOCK_COUNT
#define SIGNAL_ACCEPTANCE_BLOCK_COUNT (100U)
#endif

/**
 * 1：Demo 把 PA12/TIMG0_CCP0 配成 ZERO_EVENT 翻转输出；0：保持 PA12 为低。
 * 该开关只影响验收 Demo，不是 ADC_DMA 模块依赖。
 */
#ifndef SIGNAL_VALIDATION_TRIGGER_OUTPUT_ENABLE
#define SIGNAL_VALIDATION_TRIGGER_OUTPUT_ENABLE (1U)
#endif

#if (SIGNAL_SAMPLE_RATE_HZ == 0U)
#error "SIGNAL_SAMPLE_RATE_HZ must be greater than zero"
#endif

#if (SIGNAL_SAMPLE_COUNT == 0U) || (SIGNAL_SAMPLE_COUNT > 65535U)
#error "SIGNAL_SAMPLE_COUNT must be in the range 1..65535"
#endif

#if (SIGNAL_ACCEPTANCE_BLOCK_COUNT == 0U)
#error "SIGNAL_ACCEPTANCE_BLOCK_COUNT must be greater than zero"
#endif

#if (SIGNAL_VALIDATION_TRIGGER_OUTPUT_ENABLE != 0U) && \
    (SIGNAL_VALIDATION_TRIGGER_OUTPUT_ENABLE != 1U)
#error "SIGNAL_VALIDATION_TRIGGER_OUTPUT_ENABLE must be 0 or 1"
#endif

#endif /* SIGNAL_CONFIG_H */
