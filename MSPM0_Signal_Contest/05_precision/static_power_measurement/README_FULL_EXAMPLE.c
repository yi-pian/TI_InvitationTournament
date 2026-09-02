#include "signal_static_power.h"

void StaticPowerFullExample(float shunt_voltage_v)
{
    float current_ma = 0.0f;
    float power_mw = 0.0f;
    signal_algorithm_status_t status = SignalStaticPower_Calculate(
        shunt_voltage_v, 750.0f, 24.0f, 1.0f, &current_ma, &power_mw);
    (void)status;
    (void)current_ma;
    (void)power_mw;
}
