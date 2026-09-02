#ifndef SIGNAL_ADC_FIFO_DMA_H
#define SIGNAL_ADC_FIFO_DMA_H

/**
 * @file signal_adc_fifo_dma.h
 * @brief ADC FIFO 打包、DMA 满吞吐率单帧采集模块。
 */

#include <stdbool.h>
#include <stdint.h>

#include "signal_status.h"

/**
 * @brief ADC FIFO DMA 模块配置。
 *
 * nominal_sample_rate_hz 只记录 SysConfig 对应的名义采样率，供上层建立
 * 时间轴；它不会在运行时修改 ADC 时钟或采样时间。
 */
typedef struct {
    uint32_t nominal_sample_rate_hz;
} signal_adc_fifo_dma_config_t;

/**
 * @brief 初始化 ADC FIFO DMA 模块状态。
 * @param config 名义采样率元数据，不能为 0。
 * @return SIGNAL_RESULT_OK 表示成功。
 * @note 调用前必须先执行一次 SYSCFG_DL_init()。
 */
signal_result_t SignalADCFIFODMA_Init(
    const signal_adc_fifo_dma_config_t *config);

/**
 * @brief 以 ADC 可连续完成转换的最高吞吐方式采集一帧。
 * @param buffer DMA 目标缓冲区，必须 4 字节对齐。
 * @param sample_count uint16_t 样本数，必须为非零偶数。
 * @return SIGNAL_RESULT_OK 表示采集已经启动。
 * @note FIFO 把两个 12 位结果装入一个 32 位字，模块内部 DMA 长度为 N/2。
 */
signal_result_t SignalADCFIFODMA_Start(
    uint16_t *buffer, uint16_t sample_count);

/** @brief 立即停止采集并把模块恢复到空闲状态。 */
void SignalADCFIFODMA_Stop(void);

/** @brief 完整一帧已经搬入 RAM 时返回 true。 */
bool SignalADCFIFODMA_IsFinished(void);

/** @brief 返回 IDLE、RUNNING、DONE 或 ERROR。 */
signal_status_t SignalADCFIFODMA_GetStatus(void);

/** @brief 返回最近一次 Start 使用的缓冲区；未启动过时为空指针。 */
const uint16_t *SignalADCFIFODMA_GetBuffer(void);

/** @brief 返回最近一次 Start 的样本数；未启动过时为 0。 */
uint16_t SignalADCFIFODMA_GetSampleCount(void);

/**
 * @brief 返回 Init 记录的 SysConfig 名义采样率，单位 Hz。
 * @note 该值不是实测结果，也不代表 Init 在运行时设置了 ADC 速率。
 */
uint32_t SignalADCFIFODMA_GetNominalSampleRateHz(void);

/** @brief 返回模块证据成熟度。 */
signal_module_status_t SignalADCFIFODMA_GetModuleMaturity(void);

#endif /* SIGNAL_ADC_FIFO_DMA_H */
