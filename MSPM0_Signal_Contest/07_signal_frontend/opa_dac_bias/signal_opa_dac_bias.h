#ifndef SIGNAL_OPA_DAC_BIAS_H
#define SIGNAL_OPA_DAC_BIAS_H

#include "signal_status.h"
/**
 * @brief 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。
 * @param input_voltage_v 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。
 * @param gain `gain`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param dac_bias_v `dac_bias_v`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param output_voltage_v 由调用者分配的输出对象/数组。成功返回后才读取其中内容。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalOPADACBias_Calculate(float input_voltage_v, float gain,
    float dac_bias_v, float *output_voltage_v);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalOPADACBias_GetModuleStatus(void);

#endif

