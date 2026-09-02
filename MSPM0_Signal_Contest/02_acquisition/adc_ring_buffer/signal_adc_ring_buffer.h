#ifndef SIGNAL_ADC_RING_BUFFER_H
#define SIGNAL_ADC_RING_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

/**
 * @brief 单生产者/单消费者 ADC 环形队列。
 *
 * capacity 个存储槽只能同时保存 capacity-1 个样本；留出一个空槽用于
 * 区分“满”和“空”。Push 通常在短 ISR 中调用，Pop 在主循环调用。
 */
typedef struct {
    uint16_t *storage;
    size_t capacity;
    volatile size_t head;
    volatile size_t tail;
    volatile uint32_t overruns;
} signal_adc_ring_buffer_t;

/** @brief 绑定调用者提供的 storage；capacity 至少为 2。 */
signal_result_t SignalADCRing_Init(signal_adc_ring_buffer_t *ring,
    uint16_t *storage, size_t capacity);
/** @brief 压入一个 ADC code；满时不覆盖旧数据并返回 INSUFFICIENT_BUFFER。 */
signal_result_t SignalADCRing_Push(signal_adc_ring_buffer_t *ring,
    uint16_t sample);
/** @brief 取出最早的一个 code；空时返回 NO_DATA。 */
signal_result_t SignalADCRing_Pop(signal_adc_ring_buffer_t *ring,
    uint16_t *sample);
/** @brief 返回当前可 Pop 的样本元素数。 */
size_t SignalADCRing_Count(const signal_adc_ring_buffer_t *ring);
/** @brief 丢弃所有尚未读取的数据，并将 overrun 计数清零。 */
void SignalADCRing_Clear(signal_adc_ring_buffer_t *ring);
/** @brief 返回构建验证证据等级，不是缓冲区运行状态。 */
signal_module_status_t SignalADCRing_GetModuleStatus(void);

#endif
