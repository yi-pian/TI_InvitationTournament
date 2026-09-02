/* 最小示例：把采样波形交给占空比测量，并检查返回码。 */
#include "signal_duty.h"

static signal_algorithm_status_t MeasureDuty(
    const float *voltage_v,
    uint32_t sample_count,
    float sample_rate_hz,
    float *duty_percent)
{
    signal_duty_config_t config;
    signal_duty_result_t result;
    signal_algorithm_status_t status;

    if (duty_percent == 0)
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    status = SignalDuty_GetDefaultConfig(&config);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    status = SignalDuty_Process(
        voltage_v, sample_count, sample_rate_hz, &config, &result);
    if (status == SIGNAL_ALGORITHM_OK)
    {
        *duty_percent = result.duty_percent;
    }
    return status;
}
