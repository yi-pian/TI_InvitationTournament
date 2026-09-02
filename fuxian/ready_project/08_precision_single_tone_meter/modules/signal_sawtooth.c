#include "signal_sawtooth.h"

#include <math.h>
#include <stddef.h>

signal_result_t SignalSawtooth_GenerateWithSymmetry(
    signal_dac_wave_table_t *table,
    float offset_fraction, float amplitude_fraction, float phase_cycles,
    bool rising, float symmetry_fraction)
{
    size_t index;
    signal_result_t result = SignalDACWaveTable_Validate(table);
    if (result != SIGNAL_RESULT_OK) { return result; }
    if ((symmetry_fraction <= 0.0f) || (symmetry_fraction > 1.0f)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    for (index = 0U; index < table->count; ++index) {
        float phase = fmodf((float) index / (float) table->count +
            phase_cycles, 1.0f);
        float value;
        if (phase < 0.0f) { phase += 1.0f; }
        if (symmetry_fraction >= 1.0f) {
            /* 100%：标准上升锯齿，周期末从 +1 回落到 -1。 */
            value = -1.0f + 2.0f * phase;
        } else if (phase < symmetry_fraction) {
            value = -1.0f + 2.0f * phase / symmetry_fraction;
        } else {
            value = 1.0f - 2.0f * (phase - symmetry_fraction) /
                (1.0f - symmetry_fraction);
        }
        if (!rising) { value = -value; }
        result = SignalDACWaveTable_NormalizedToRaw(value, table->dac_bits,
            offset_fraction, amplitude_fraction, &table->samples[index]);
        if (result != SIGNAL_RESULT_OK) { return result; }
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalSawtooth_Generate(signal_dac_wave_table_t *table,
    float offset_fraction, float amplitude_fraction, float phase_cycles,
    bool rising)
{
    return SignalSawtooth_GenerateWithSymmetry(table, offset_fraction,
        amplitude_fraction, phase_cycles, rising, 1.0f);
}

signal_module_status_t SignalSawtooth_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
