#include "signal_dac_wave_table.h"

#include <stddef.h>
#include "signal_math.h"

signal_result_t SignalDACWaveTable_Validate(
    const signal_dac_wave_table_t *table)
{
    if ((table == NULL) || (table->samples == NULL) || (table->count < 2U) ||
        (table->dac_bits == 0U) || (table->dac_bits > 16U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalDACWaveTable_NormalizedToRaw(float normalized,
    uint8_t dac_bits, float offset_fraction, float amplitude_fraction,
    uint16_t *raw)
{
    uint32_t full_scale;
    float fraction;
    if ((raw == NULL) || (dac_bits == 0U) || (dac_bits > 16U) ||
        (offset_fraction < 0.0f) || (offset_fraction > 1.0f) ||
        (amplitude_fraction < 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    fraction = offset_fraction + amplitude_fraction *
        SignalMath_ClampF32(normalized, -1.0f, 1.0f);
    if ((fraction < 0.0f) || (fraction > 1.0f)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    full_scale = (1UL << dac_bits) - 1UL;
    *raw = (uint16_t) (fraction * (float) full_scale + 0.5f);
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalDACWaveTable_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
