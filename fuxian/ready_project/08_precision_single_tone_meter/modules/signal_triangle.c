#include "signal_triangle.h"

#include <math.h>
#include <stddef.h>

signal_result_t SignalTriangle_Generate(signal_dac_wave_table_t *table,
    float offset_fraction, float amplitude_fraction, float phase_cycles)
{
    size_t index;
    signal_result_t result = SignalDACWaveTable_Validate(table);
    if (result != SIGNAL_RESULT_OK) { return result; }
    for (index = 0U; index < table->count; ++index) {
        float phase = fmodf((float) index / (float) table->count +
            phase_cycles, 1.0f);
        float value;
        if (phase < 0.0f) { phase += 1.0f; }
        value = 1.0f - 4.0f * fabsf(phase - 0.5f);
        result = SignalDACWaveTable_NormalizedToRaw(value, table->dac_bits,
            offset_fraction, amplitude_fraction, &table->samples[index]);
        if (result != SIGNAL_RESULT_OK) { return result; }
    }
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalTriangle_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
