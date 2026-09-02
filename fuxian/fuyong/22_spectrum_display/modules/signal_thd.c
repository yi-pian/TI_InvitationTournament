#include "signal_thd.h"

#include <math.h>
#include <stddef.h>

signal_algorithm_status_t SignalTHD_Process(
    const signal_harmonic_result_t *harmonics,
    signal_thd_result_t *result)
{
    uint32_t order;
    float harmonic_energy_sum = 0.0f;
    float fundamental_energy;

    if ((harmonics == NULL) || (result == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((harmonics->first_order != 1U) || (harmonics->last_order < 2U) ||
        (harmonics->last_order > SIGNAL_HARMONIC_MAX_ORDER))
    {
        return SIGNAL_ALGORITHM_INSUFFICIENT_DATA;
    }
    fundamental_energy = harmonics->items[1U].energy;
    if (!isfinite(fundamental_energy) || (fundamental_energy <= 0.0f))
    {
        return SIGNAL_ALGORITHM_NO_FEATURE;
    }
    for (order = 2U; order <= harmonics->last_order; ++order)
    {
        float energy = harmonics->items[order].energy;
        if (!isfinite(energy) || (energy < 0.0f))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        harmonic_energy_sum += energy;
    }
    result->fundamental_energy = fundamental_energy;
    result->harmonic_energy_sum = harmonic_energy_sum;
    result->thd_ratio = sqrtf(harmonic_energy_sum / fundamental_energy);
    result->thd_percent = 100.0f * result->thd_ratio;
    return (isfinite(result->thd_ratio) && isfinite(result->thd_percent))
               ? SIGNAL_ALGORITHM_OK
               : SIGNAL_ALGORITHM_NUMERIC_ERROR;
}
