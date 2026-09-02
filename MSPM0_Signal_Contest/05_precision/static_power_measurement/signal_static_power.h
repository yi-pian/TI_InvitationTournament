#ifndef SIGNAL_STATIC_POWER_H
#define SIGNAL_STATIC_POWER_H

#include "signal_algorithm_status.h"

signal_algorithm_status_t SignalStaticPower_Calculate(
    float shunt_voltage_v, float shunt_resistance_ohm,
    float supply_voltage_v, float rail_count_factor,
    float *current_ma, float *power_mw);

#endif
