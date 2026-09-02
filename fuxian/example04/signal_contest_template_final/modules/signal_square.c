#include "signal_square.h"

#include <math.h>
#include <stddef.h>

signal_result_t SignalSquare_GenerateWithDuty(signal_dac_wave_table_t *table,
    float offset_fraction, float amplitude_fraction, float duty_fraction,
    float phase_cycles)
{
    size_t index;
    signal_result_t result = SignalDACWaveTable_Validate(table);
    if (result != SIGNAL_RESULT_OK) { return result; }
    if ((duty_fraction <= 0.0f) || (duty_fraction >= 1.0f)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    for (index = 0U; index < table->count; ++index) {
        float phase = fmodf((float) index / (float) table->count +
            phase_cycles, 1.0f);
        if (phase < 0.0f) { phase += 1.0f; }
        result = SignalDACWaveTable_NormalizedToRaw(
            (phase < duty_fraction) ? 1.0f : -1.0f, table->dac_bits,
            offset_fraction, amplitude_fraction, &table->samples[index]);
        if (result != SIGNAL_RESULT_OK) { return result; }
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalSquare_Generate(signal_dac_wave_table_t *table,
    float offset_fraction, float amplitude_fraction, float duty_fraction,
    float phase_cycles)
{
    return SignalSquare_GenerateWithDuty(table, offset_fraction,
        amplitude_fraction, duty_fraction, phase_cycles);
}

signal_module_status_t SignalSquare_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
