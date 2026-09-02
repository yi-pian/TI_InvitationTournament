/* 最小示例：DMA 中断交替填 A/B，主循环处理已完成的一块。 */
#include <stddef.h>
#include <stdint.h>

#include "signal_adc_pingpong_dma.h"

/* 这里通常改成一次 DMA block 的 ADC 样本数。 */
#define BLOCK_SAMPLE_COUNT (128U)

static uint16_t g_buffer_a[BLOCK_SAMPLE_COUNT];
static uint16_t g_buffer_b[BLOCK_SAMPLE_COUNT];
static signal_adc_pingpong_dma_t g_pingpong;

void SignalADCPingPong_MinimalExample(void)
{
    const uint16_t *ready_samples;
    uint16_t *next_dma_destination;
    signal_pingpong_buffer_id_t ready_id;
    size_t ready_count;

    if (SignalADCPingPong_Init(&g_pingpong, g_buffer_a, g_buffer_b,
            BLOCK_SAMPLE_COUNT) != SIGNAL_RESULT_OK) {
        return;
    }

    /* 真实 DMA 首次目标应设为 g_buffer_a。这里模拟 A 块已经传输完成。 */
    if (SignalADCPingPong_OnDmaComplete(&g_pingpong,
            &next_dma_destination) == SIGNAL_RESULT_OK) {
        /* 将 next_dma_destination 写回 DMA destination 后，DMA 可开始填 B。 */
        (void)next_dma_destination;
    }

    if (SignalADCPingPong_Acquire(&g_pingpong, &ready_id, &ready_samples,
            &ready_count) == SIGNAL_RESULT_OK) {
        (void)ready_samples;
        (void)ready_count;
        /* ===== 从这里开始处理完整的一帧数据 ===== */
        (void)SignalADCPingPong_Release(&g_pingpong, ready_id);
    }
}
