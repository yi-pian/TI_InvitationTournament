#ifndef SIGNAL_DAC_DMA_H
#define SIGNAL_DAC_DMA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

typedef signal_result_t (*signal_dac_dma_start_fn)(void *context,
    const uint16_t *samples, size_t count, bool repeat);
typedef signal_result_t (*signal_dac_dma_stop_fn)(void *context);

typedef struct {
    void *context;
    signal_dac_dma_start_fn start;
    signal_dac_dma_stop_fn stop;
    signal_status_t state;
    size_t active_count;
    bool repeat;
} signal_dac_dma_t;
/**
 * @brief 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。
 * @param module `module`（`signal_dac_dma_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param context 传给平台回调的用户上下文，由应用创建并保证在调用期间有效。
 * @param start `start`（`signal_dac_dma_start_fn`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param stop `stop`（`signal_dac_dma_stop_fn`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalDACDMA_Init(signal_dac_dma_t *module, void *context,
    signal_dac_dma_start_fn start, signal_dac_dma_stop_fn stop);
/**
 * @brief 启动一轮新的硬件操作或异步传输；成功后按对应的完成查询 API 等待结果。
 * @param module `module`（`signal_dac_dma_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param samples 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。
 * @param count 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @param repeat `repeat`（`bool`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */
signal_result_t SignalDACDMA_Start(signal_dac_dma_t *module,
    const uint16_t *samples, size_t count, bool repeat);
/**
 * @brief 主动终止当前操作并释放模块占用的运行状态；只在需要取消本轮任务时调用。
 * @param module `module`（`signal_dac_dma_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */
signal_result_t SignalDACDMA_Stop(signal_dac_dma_t *module);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalDACDMA_GetModuleStatus(void);

#endif

