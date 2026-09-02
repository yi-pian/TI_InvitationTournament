#ifndef SIGNAL_DAC_WAVE_TABLE_H
#define SIGNAL_DAC_WAVE_TABLE_H

#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

typedef struct {
    uint16_t *samples;
    size_t count;
    uint8_t dac_bits;
} signal_dac_wave_table_t;
/**
 * @brief 检查配置或输入是否满足模块要求；建议在第一次接入或参数变化后调用。
 * @param table 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalDACWaveTable_Validate(
    const signal_dac_wave_table_t *table);
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param normalized `normalized`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param dac_bits `dac_bits`（`uint8_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param offset_fraction `offset_fraction`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param amplitude_fraction `amplitude_fraction`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param raw `raw`（`uint16_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */
signal_result_t SignalDACWaveTable_NormalizedToRaw(float normalized,
    uint8_t dac_bits, float offset_fraction, float amplitude_fraction,
    uint16_t *raw);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalDACWaveTable_GetModuleStatus(void);

#endif

