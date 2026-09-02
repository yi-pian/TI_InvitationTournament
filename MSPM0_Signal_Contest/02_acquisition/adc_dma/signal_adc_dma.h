#ifndef SIGNAL_ADC_DMA_H
#define SIGNAL_ADC_DMA_H

/**
 * @file signal_adc_dma.h
 * @brief 定时器事件触发 ADC、DMA 搬运到 RAM 的单通道采集模块。
 */

#include <stdbool.h>
#include <stdint.h>

#include "signal_status.h"

/**
 * @brief ADC+DMA 模块运行配置。
 *
 * timer_clock_hz 必须等于 SysConfig 中定时器经过时钟源、分频器和
 * 预分频器后的真实计数时钟。本阶段 Demo 使用 BUSCLK / 1 / 1。
 */
typedef struct {
    uint32_t sample_rate_hz;
    uint32_t timer_clock_hz;
    uint32_t timer_max_count;
} signal_adc_dma_config_t;

/**
 * @brief 初始化 ADC+DMA 模块的运行时状态。
 * @param config 采样率、定时器计数时钟和最大周期计数。
 * @return SIGNAL_RESULT_OK 表示成功；其他值表示参数或范围错误。
 * @note 调用前必须先执行一次 SYSCFG_DL_init()。
 */
signal_result_t SignalADC_Init(const signal_adc_dma_config_t *config);

/**
 * @brief 修改目标采样事件率并计算最近的整数 Timer 配置触发率。
 * @param sample_rate_hz 目标采样事件率，单位 Hz。
 * @return SIGNAL_RESULT_OK 表示成功；运行中返回 SIGNAL_RESULT_BUSY。
 * @note 只修改定时器周期，不修改 ADC 通道和 Event Fabric 路由。
 */
signal_result_t SignalADC_SetSampleRate(uint32_t sample_rate_hz);

/**
 * @brief 启动一次 N 点采集。
 * @param buffer DMA 目标缓冲区，元素类型必须为 uint16_t。
 * @param sample_count 采样点数，范围 1~65535。
 * @return SIGNAL_RESULT_OK 表示已启动。
 * @note 缓冲区容量至少为 sample_count 个 uint16_t，采集完成前不能释放。
 */
signal_result_t SignalADC_Start(uint16_t *buffer, uint16_t sample_count);

/**
 * @brief 立即停止当前采集并把模块恢复为空闲状态。
 * @return 无。
 */
void SignalADC_Stop(void);

/**
 * @brief 查询本次采集是否完成。
 * @return 完成返回 true，否则返回 false。
 */
bool SignalADC_IsFinished(void);

/**
 * @brief 查询模块当前状态。
 * @return MODULE_IDLE、MODULE_RUNNING、MODULE_DONE 或 MODULE_ERROR。
 */
signal_status_t SignalADC_GetStatus(void);

/**
 * @brief 取得最近一次采集使用的缓冲区。
 * @return 缓冲区只读指针；尚未启动过采集时返回空指针。
 */
const uint16_t *SignalADC_GetBuffer(void);

/**
 * @brief 取得最近一次采集的点数。
 * @return 采样点数；尚未启动过采集时返回 0。
 */
uint16_t SignalADC_GetSampleCount(void);

/**
 * @brief 取得由 Timer 整数计数配置推导出的事件触发率。
 * @return 配置触发率，单位 Hz；初始化失败时返回 0。
 * @note 该值仅由 timer_clock_hz、分频/预分频结果和 Timer load 计算得到，
 *       不是示波器或其他外部仪器实测的 ADC 物理采样率，也不补偿时钟误差、
 *       Event 传播、ADC 采样孔径或潜在的丢触发。
 */
uint32_t SignalADC_GetConfiguredTriggerRate(void);

/** @brief 返回模块证据成熟度；当前来自 2026-08-07 板载 TMP6131 验收。 */
signal_module_status_t SignalADC_GetModuleMaturity(void);

#endif /* SIGNAL_ADC_DMA_H */
