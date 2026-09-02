#include "signal_sine.h"

#include <math.h>
#include <stddef.h>
#include "signal_math.h"

signal_result_t SignalSine_Generate(signal_dac_wave_table_t *table,
    float offset_fraction, float amplitude_fraction, float phase_cycles)
{
    size_t index;
    signal_result_t result = SignalDACWaveTable_Validate(table);
    if (result != SIGNAL_RESULT_OK) { return result; }
    for (index = 0U; index < table->count; ++index) {
        float phase = (float) index / (float) table->count + phase_cycles;
        result = SignalDACWaveTable_NormalizedToRaw(
            sinf(SIGNAL_TWO_PI_F * phase), table->dac_bits,
            offset_fraction, amplitude_fraction, &table->samples[index]);
        if (result != SIGNAL_RESULT_OK) { return result; }
    }
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalSine_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
