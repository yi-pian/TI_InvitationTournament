#ifndef SIGNAL_OPA_TO_ADC_H
#define SIGNAL_OPA_TO_ADC_H

#include "signal_status.h"

typedef struct {
    float expected_min_v;
    float expected_max_v;
    float adc_low_limit_v;
    float adc_high_limit_v;
} signal_opa_to_adc_budget_t;
/**
 * @brief 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。
 * @param budget `budget`（`const signal_opa_to_adc_budget_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param low_margin_v `low_margin_v`（`float *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @param high_margin_v `high_margin_v`（`float *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。
 * @return 返回 signal_result_t 类型结果；调用者应检查该值。
 */

signal_result_t SignalOPAToADC_CheckRange(
    const signal_opa_to_adc_budget_t *budget, float *low_margin_v,
    float *high_margin_v);
/**
 * @brief 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。
 * @return 返回 signal_module_status_t 类型结果；调用者应检查该值。
 */
signal_module_status_t SignalOPAToADC_GetModuleStatus(void);

#endif

