#ifndef SIGNAL_ADC_PINGPONG_DMA_H
#define SIGNAL_ADC_PINGPONG_DMA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

/** @brief 两块应用缓冲区的编号。 */
typedef enum {
    SIGNAL_PINGPONG_BUFFER_A = 0,
    SIGNAL_PINGPONG_BUFFER_B = 1
} signal_pingpong_buffer_id_t;

/**
 * @brief Ping-Pong 缓冲区的软件状态。
 *
 * DMA 完成中断只应调用 OnDmaComplete() 并立刻把 next_destination 写给
 * 硬件；主循环用 Acquire()/Release() 消费已经就绪的块。
 */
typedef struct {
    uint16_t *buffer[2];
    size_t sample_count;
    volatile bool ready[2];
    volatile signal_pingpong_buffer_id_t dma_target;
    uint32_t completed_blocks;
    uint32_t overrun_blocks;
} signal_adc_pingpong_dma_t;

/**
 * @brief 绑定两块不同的等长缓冲区，并把 DMA 下一目标设为 A。
 * @param sample_count 每块的 uint16_t 元素数，必须非零。
 */
signal_result_t SignalADCPingPong_Init(signal_adc_pingpong_dma_t *module,
    uint16_t *buffer_a, uint16_t *buffer_b, size_t sample_count);
/**
 * @brief 在一块 DMA 完成时标记该块就绪并给出下一块目标地址。
 * @param next_destination 成功时返回应立即装入 DMA 的另一块缓冲区地址。
 * @return BUSY 表示下一块尚未被应用 Release，DMA 若继续写会覆盖未处理数据。
 */
signal_result_t SignalADCPingPong_OnDmaComplete(signal_adc_pingpong_dma_t *module,
    uint16_t **next_destination);
/**
 * @brief 取得一块 DMA 已写完、可由主循环只读处理的缓冲区。
 * @return NO_DATA 表示当前没有完整块；成功后必须以同一 id 调用 Release。
 */
signal_result_t SignalADCPingPong_Acquire(signal_adc_pingpong_dma_t *module,
    signal_pingpong_buffer_id_t *id, const uint16_t **samples, size_t *count);
/** @brief 声明一块已处理完，可再次交给 DMA 覆盖。 */
signal_result_t SignalADCPingPong_Release(signal_adc_pingpong_dma_t *module,
    signal_pingpong_buffer_id_t id);
/** @brief 返回构建验证证据等级，不是 DMA 实时状态。 */
signal_module_status_t SignalADCPingPong_GetModuleStatus(void);

#endif
