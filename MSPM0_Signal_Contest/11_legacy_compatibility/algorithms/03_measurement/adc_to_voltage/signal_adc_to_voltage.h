#ifndef SIGNAL_ADC_TO_VOLTAGE_H
#define SIGNAL_ADC_TO_VOLTAGE_H

#include <stdint.h>

#include "signal_algorithm_status.h"

typedef struct
{
    uint32_t adc_max_code;
    float reference_voltage_v;
    float input_scale;
    float offset_voltage_v;
} signal_adc_to_voltage_config_t;

/**
 * @brief 把无符号 ADC 原始码逐点换算成电压。
 * @param raw_codes ADC RAW 输入数组，单位 code，范围应为 0~adc_max_code。
 * @param voltage_v 输出数组，单位 V，容量至少为 count 个 float。
 * @param count 样本点数，必须大于 0。
 * @param config 换算配置；公式为 raw/adc_max_code*reference_voltage_v*input_scale+offset_voltage_v。
 * @return 成功返回 SIGNAL_ALGORITHM_OK；参数或码值非法时返回对应错误码。
 * @note 输入与输出由调用者管理；函数不使用动态内存，也不访问 ADC 寄存器。
 */
signal_algorithm_status_t SignalADCToVoltage_Process(
    const uint16_t *raw_codes,
    float *voltage_v,
    uint32_t count,
    const signal_adc_to_voltage_config_t *config);

#endif /* SIGNAL_ADC_TO_VOLTAGE_H */
