/* 全功能示例：展示 Ping-Pong 状态模块的所有公开 API。 */
#include <stddef.h>
#include <stdint.h>

#include "signal_adc_pingpong_dma.h"

#define BLOCK_SAMPLE_COUNT (128U)

static uint16_t g_buffer_a[BLOCK_SAMPLE_COUNT];
static uint16_t g_buffer_b[BLOCK_SAMPLE_COUNT];
static signal_adc_pingpong_dma_t g_pingpong;

void SignalADCPingPong_FullExample(void)
{
    const uint16_t *samples;
    uint16_t *next_dma_destination;
    signal_pingpong_buffer_id_t id;
    size_t count;
    signal_result_t result;
    signal_module_status_t maturity;

    result = SignalADCPingPong_Init(&g_pingpong, g_buffer_a, g_buffer_b,
        BLOCK_SAMPLE_COUNT);
    if (result != SIGNAL_RESULT_OK) {
        return;
    }

    maturity = SignalADCPingPong_GetModuleStatus();
    (void)maturity;

    /* 仅在 DMA 完成 ISR 中调用：它会返回下一块给 DMA。 */
    result = SignalADCPingPong_OnDmaComplete(&g_pingpong,
        &next_dma_destination);
    if (result == SIGNAL_RESULT_OK) {
        (void)next_dma_destination;
    }

    /* 在主循环获取完整块、只读处理，并及时归还。 */
    result = SignalADCPingPong_Acquire(&g_pingpong, &id, &samples, &count);
    if (result == SIGNAL_RESULT_OK) {
        (void)samples;
        (void)count;
        (void)SignalADCPingPong_Release(&g_pingpong, id);
    }
}
