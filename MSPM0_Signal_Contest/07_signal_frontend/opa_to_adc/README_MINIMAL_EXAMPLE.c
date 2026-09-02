/* OPA 到 ADC：检查预期 0.45 V 到 2.85 V 是否适合 0 V 到 3.3 V ADC。 */
#include "signal_opa_to_adc.h"

void opa_to_adc_MinimalExample(void)
{
    const signal_opa_to_adc_budget_t budget = {
        .expected_min_v = 0.45f,
        .expected_max_v = 2.85f,
        .adc_low_limit_v = 0.0f,
        .adc_high_limit_v = 3.3f
    };
    float low_margin_v = 0.0f;
    float high_margin_v = 0.0f;
    signal_result_t result = SignalOPAToADC_CheckRange(
        &budget, &low_margin_v, &high_margin_v);

    /* 成功时两侧余量均为 0.45 V。 */
    if (result != SIGNAL_RESULT_OK) {
        return;
    }
}
