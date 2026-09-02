/* 最小示例：生产者写入 ADC code，主循环按先进先出读取。 */
#include <stdint.h>

#include "signal_adc_ring_buffer.h"

/* 实际可保存的样本数是 RING_CAPACITY - 1，即 7 个。 */
#define RING_CAPACITY (8U)

static uint16_t g_storage[RING_CAPACITY];
static signal_adc_ring_buffer_t g_ring;

void SignalADCRing_MinimalExample(void)
{
    uint16_t sample;

    if (SignalADCRing_Init(&g_ring, g_storage, RING_CAPACITY) !=
        SIGNAL_RESULT_OK) {
        return;
    }
    /* 实际工程中这一句通常在 ADC 完成 ISR 中调用。 */
    (void)SignalADCRing_Push(&g_ring, 2048U);
    if (SignalADCRing_Pop(&g_ring, &sample) == SIGNAL_RESULT_OK) {
        (void)sample; /* ===== 在这里使用一个已采样点 ===== */
    }
}
