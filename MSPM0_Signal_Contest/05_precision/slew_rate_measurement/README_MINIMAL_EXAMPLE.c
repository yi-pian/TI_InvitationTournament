#include "signal_slew_rate.h"

signal_slew_rate_result_t App_MeasureSlew(const float *samples, uint32_t count)
{
    const signal_slew_rate_config_t config = {0.20f, 0.80f};
    signal_slew_rate_result_t result = {0};
    (void)SignalSlewRate_Process(samples, count, 0.0f, 3.0f, 2000000U,
        &config, &result);
    return result;
}
