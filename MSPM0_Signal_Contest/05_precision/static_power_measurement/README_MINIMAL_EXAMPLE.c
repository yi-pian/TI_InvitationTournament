#include "signal_static_power.h"

void App_CalculatePower(float shunt_voltage_v)
{
    float current_ma;
    float power_mw;
    (void)SignalStaticPower_Calculate(shunt_voltage_v, 750.0f, 24.0f, 1.0f,
        &current_ma, &power_mw);
}
