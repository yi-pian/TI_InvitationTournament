#include "signal_static_power.h"

signal_algorithm_status_t SignalStaticPower_Calculate(
    float shunt_voltage_v, float shunt_resistance_ohm,
    float supply_voltage_v, float rail_count_factor,
    float *current_ma, float *power_mw)
{
    float current_a;
    if ((current_ma == 0) || (power_mw == 0) ||
        (shunt_resistance_ohm <= 0.0f) || (supply_voltage_v < 0.0f) ||
        (rail_count_factor < 0.0f)) {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (shunt_voltage_v < 0.0f) shunt_voltage_v = 0.0f;
    current_a = shunt_voltage_v / shunt_resistance_ohm;
    *current_ma = current_a * 1000.0f;
    *power_mw = current_a * supply_voltage_v * rail_count_factor * 1000.0f;
    return SIGNAL_ALGORITHM_OK;
}
