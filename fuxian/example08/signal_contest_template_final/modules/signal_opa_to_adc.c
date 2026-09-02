#include "signal_opa_to_adc.h"

#include <stddef.h>

signal_result_t SignalOPAToADC_CheckRange(
    const signal_opa_to_adc_budget_t *budget, float *low_margin_v,
    float *high_margin_v)
{
    if ((budget == NULL) || (low_margin_v == NULL) ||
        (high_margin_v == NULL) ||
        !(budget->adc_high_limit_v > budget->adc_low_limit_v) ||
        (budget->expected_max_v < budget->expected_min_v)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *low_margin_v = budget->expected_min_v - budget->adc_low_limit_v;
    *high_margin_v = budget->adc_high_limit_v - budget->expected_max_v;
    return ((*low_margin_v >= 0.0f) && (*high_margin_v >= 0.0f)) ?
        SIGNAL_RESULT_OK : SIGNAL_RESULT_OUT_OF_RANGE;
}

signal_module_status_t SignalOPAToADC_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
