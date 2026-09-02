#ifndef SIGNAL_BACKEND_ADAPTER_H
#define SIGNAL_BACKEND_ADAPTER_H

#include <stdint.h>

#include "signal_algorithm_status.h"

/**
 * @brief 把以 full_scale 为满量程的 float 样本转换成 Q15。
 * @param input_samples 输入样本，只读；单位由调用者决定。
 * @param output_q15 输出有符号 Q15，-32768 表示 -1，32767 约等于 +1。
 * @param count 样本数量，必须大于 0。
 * @param full_scale 输入中对应归一化幅值 1.0 的正数，单位与输入相同。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；非有限数或非法参数返回错误码。
 * @note 超过满量程的输入会饱和，不会发生整数回绕；允许原始输入被后续覆盖，但不支持重叠数组。
 */
signal_algorithm_status_t SignalBackendAdapter_FloatToQ15(
    const float *input_samples,
    int16_t *output_q15,
    uint32_t count,
    float full_scale);

/**
 * @brief 把 Q15 样本恢复为 float 物理量。
 * @param input_q15 输入 Q15 样本，只读。
 * @param output_samples 输出 float 样本，单位由 full_scale 决定。
 * @param count 样本数量，必须大于 0。
 * @param full_scale Q15 幅值 1.0 对应的物理满量程，必须为正数。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 */
signal_algorithm_status_t SignalBackendAdapter_Q15ToFloat(
    const int16_t *input_q15,
    float *output_samples,
    uint32_t count,
    float full_scale);

/**
 * @brief 将无符号 ADC RAW 直接居中并映射到 Q15，避免先生成整块 float 数组。
 * @param adc_raw ADC 原始码数组，例如 12 位 ADC 的 0..4095。
 * @param output_q15 输出 Q15 数组。
 * @param count 样本数量，必须大于 0。
 * @param zero_code 输入物理量为 0 时的 ADC 码，例如双极性前端中点 2048。
 * @param positive_span_codes 从 zero_code 到正满量程的码数，必须大于 0。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；超过 int16 可表示范围的配置返回错误码。
 * @note 结果会饱和到 [-32768,32767]；本函数不知道 ADC 寄存器和 DMA。
 */
signal_algorithm_status_t SignalBackendAdapter_ADCRawToQ15(
    const uint16_t *adc_raw,
    int16_t *output_q15,
    uint32_t count,
    uint16_t zero_code,
    uint16_t positive_span_codes);

/**
 * @brief 累加 Q15 样本平方，供 RMS/能量算法继续使用。
 * @param input_q15 输入 Q15 样本。
 * @param count 样本数量，必须大于 0。
 * @param sum_squares_q30 输出平方和；每一项为 Q30，累加器为 uint64_t。
 * @return 成功返回 SIGNAL_ALGORITHM_OK。
 * @note 32768 个满幅样本仍不会溢出 uint64_t；更长数据需由调用者检查累计上限。
 */
signal_algorithm_status_t SignalBackendAdapter_Q15SquareAccumulate(
    const int16_t *input_q15,
    uint32_t count,
    uint64_t *sum_squares_q30);

#endif /* SIGNAL_BACKEND_ADAPTER_H */
