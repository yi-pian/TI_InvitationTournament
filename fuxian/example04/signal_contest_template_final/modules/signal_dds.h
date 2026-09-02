#ifndef SIGNAL_DDS_H
#define SIGNAL_DDS_H

#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

typedef struct {
    uint32_t phase_accumulator;
    uint32_t tuning_word;
    const uint16_t *table;
    size_t table_count;
} signal_dds_t;
/**
 * @brief 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。
 * @param dds `dds`（`signal_dds_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param table 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。
 * @param table_count 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。
 * @param output_frequency_hz 由调用者分配的输出对象/数组。成功返回后才读取其中内容。
 * @param update_rate_hz 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。
 * @param initial_phase `initial_phase`（`uint32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalDDS_Init(signal_dds_t *dds, const uint16_t *table,
    size_t table_count, float output_frequency_hz, float update_rate_hz,
    uint32_t initial_phase);
/**
 * @brief 修改模块的一个运行参数；若模块有 BUSY/RUNNING 状态，应在空闲时修改。
 * @param dds `dds`（`signal_dds_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param output_frequency_hz 由调用者分配的输出对象/数组。成功返回后才读取其中内容。
 * @param update_rate_hz 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */
signal_result_t SignalDDS_SetFrequency(signal_dds_t *dds,
    float output_frequency_hz, float update_rate_hz);
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param dds `dds`（`signal_dds_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 uint16_t 类型结果；调用者应检查该值。
 */
uint16_t SignalDDS_Next(signal_dds_t *dds);
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param dds `dds`（`signal_dds_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param output 由调用者分配的输出对象/数组。成功返回后才读取其中内容。
 * @param count 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */
signal_result_t SignalDDS_Fill(signal_dds_t *dds, uint16_t *output,
    size_t count);
/**
 * @brief 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。
 * @param dds `dds`（`const signal_dds_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param update_rate_hz 频率/速率参数，单位 Hz。必须传入实际配置或测得的上游数值，不能把 ADC 时钟名称直接当采样率。
 * @return 返回 float 类型结果；调用者应检查该值。
 */
float SignalDDS_GetConfiguredFrequency(const signal_dds_t *dds,
    float update_rate_hz);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalDDS_GetModuleStatus(void);

#endif

