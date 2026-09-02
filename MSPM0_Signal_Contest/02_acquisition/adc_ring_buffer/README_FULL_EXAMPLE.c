/* 全功能示例：展示环形缓冲区的六个公开 API。 */
#include <stddef.h>
#include <stdint.h>

#include "signal_adc_ring_buffer.h"

#define RING_CAPACITY (8U)

static uint16_t g_storage[RING_CAPACITY];
static signal_adc_ring_buffer_t g_ring;

void SignalADCRing_FullExample(void)
{
    uint16_t sample;
    size_t pending;
    signal_module_status_t maturity;

    if (SignalADCRing_Init(&g_ring, g_storage, RING_CAPACITY) !=
        SIGNAL_RESULT_OK) {
        return;
    }
    (void)SignalADCRing_Push(&g_ring, 2048U);
    pending = SignalADCRing_Count(&g_ring);
    if ((pending != 0U) &&
        (SignalADCRing_Pop(&g_ring, &sample) == SIGNAL_RESULT_OK)) {
        (void)sample;
    }
    SignalADCRing_Clear(&g_ring); /* 例如切换量程后丢弃旧样本。 */
    maturity = SignalADCRing_GetModuleStatus();
    (void)maturity;
}
