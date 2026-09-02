/* OPA 到 ADC：展示当前头文件的全部公开 API。 */
#include "signal_opa_to_adc.h"

void opa_to_adc_FullExample(void)
{
    const signal_opa_to_adc_budget_t budget = {
        .expected_min_v = 0.45f,
        .expected_max_v = 2.85f,
        .adc_low_limit_v = 0.0f,
        .adc_high_limit_v = 3.3f
    };
    float low_margin_v = 0.0f;
    float high_margin_v = 0.0f;
    signal_module_status_t status = SignalOPAToADC_GetModuleStatus();
    signal_result_t result = SignalOPAToADC_CheckRange(
        &budget, &low_margin_v, &high_margin_v);

    /* 这个 helper 不会配置硬件；通过后仍需分别配置 OPA 和 ADC。 */
    if ((status != MODULE_STATUS_BUILD_VERIFIED) ||
        (result != SIGNAL_RESULT_OK)) {
        return;
    }
}
