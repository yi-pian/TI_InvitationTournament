#include "signal_slew_rate.h"

void SlewRateFullExample(const float *samples, uint32_t count)
{
    const signal_slew_rate_config_t config = {
        .low_ratio = 0.20f, .high_ratio = 0.80f
    };
    signal_slew_rate_result_t result;
    signal_algorithm_status_t status = SignalSlewRate_Process(
        samples, count, 0.0f, 3.0f, 2000000U, &config, &result);
    (void)status;
    (void)result;
}
