#include <stdint.h>

#include "signal_duty.h"

static float s_wave[40];
static signal_duty_result_t s_result;

int main(void)
{
    signal_duty_config_t config;
    signal_algorithm_status_t status;
    uint32_t index;

    for (index = 0U; index < 40U; ++index)
    {
        uint32_t phase = index % 8U;
        s_wave[index] = ((phase == 1U) || (phase == 2U)) ? 1.0F : 0.0F;
    }
    status = SignalDuty_GetDefaultConfig(&config);
    if (status == SIGNAL_ALGORITHM_OK)
    {
        status = SignalDuty_Process(s_wave, 40U, 1000.0F, &config, &s_result);
    }
    return (status == SIGNAL_ALGORITHM_OK) ? 0 : 1;
}
