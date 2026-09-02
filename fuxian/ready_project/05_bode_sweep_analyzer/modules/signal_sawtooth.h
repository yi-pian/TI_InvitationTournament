#ifndef SIGNAL_SAWTOOTH_H
#define SIGNAL_SAWTOOTH_H

#include <stdbool.h>
#include "signal_dac_wave_table.h"
/**
 * @brief 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。
 * @param table 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。
 * @param offset_fraction `offset_fraction`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param amplitude_fraction `amplitude_fraction`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param phase_cycles `phase_cycles`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param rising `rising`（`bool`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalSawtooth_GenerateWithSymmetry(
    signal_dac_wave_table_t *table,
    float offset_fraction, float amplitude_fraction, float phase_cycles,
    bool rising, float symmetry_fraction);

/* 兼容旧调用：固定采用 50% 上升段比例。 */
signal_result_t SignalSawtooth_Generate(signal_dac_wave_table_t *table,
    float offset_fraction, float amplitude_fraction, float phase_cycles,
    bool rising);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalSawtooth_GetModuleStatus(void);

#endif

