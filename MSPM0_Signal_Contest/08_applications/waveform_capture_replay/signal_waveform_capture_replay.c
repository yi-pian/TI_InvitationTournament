#include "signal_waveform_capture_replay.h"

#include <stddef.h>
#include "signal_arbitrary_wave.h"

signal_result_t SignalWaveformReplay_Prepare(const uint16_t *captured,
    size_t captured_count, uint16_t captured_min, uint16_t captured_max,
    uint8_t dac_bits, uint16_t *dac_table, size_t dac_table_count)
{
    size_t index;
    uint32_t full_scale;
    signal_result_t status;
    if ((captured == NULL) || (dac_table == NULL) ||
        (captured_max <= captured_min) || (dac_bits == 0U) ||
        (dac_bits > 16U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    status = SignalArbitraryWave_ResampleLinear(captured, captured_count,
        dac_table, dac_table_count);
    if (status != SIGNAL_RESULT_OK) { return status; }
    full_scale = (1UL << dac_bits) - 1UL;
    for (index = 0U; index < dac_table_count; ++index) {
        uint32_t clamped = dac_table[index];
        if (clamped < captured_min) { clamped = captured_min; }
        if (clamped > captured_max) { clamped = captured_max; }
        dac_table[index] = (uint16_t) (((clamped - captured_min) * full_scale +
            (captured_max - captured_min) / 2U) /
            (captured_max - captured_min));
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalWaveformReplay_PrepareAutoRange(
    const uint16_t *captured, size_t captured_count, uint8_t dac_bits,
    uint16_t *dac_table, size_t dac_table_count, uint16_t *captured_min,
    uint16_t *captured_max)
{
    size_t index;
    uint16_t minimum;
    uint16_t maximum;
    if ((captured == NULL) || (captured_count < 2U) ||
        (captured_min == NULL) || (captured_max == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    minimum = captured[0];
    maximum = captured[0];
    for (index = 1U; index < captured_count; ++index) {
        if (captured[index] < minimum) { minimum = captured[index]; }
        if (captured[index] > maximum) { maximum = captured[index]; }
    }
    if (maximum <= minimum) { return SIGNAL_RESULT_NO_DATA; }
    *captured_min = minimum;
    *captured_max = maximum;
    return SignalWaveformReplay_Prepare(captured, captured_count,
        minimum, maximum, dac_bits, dac_table, dac_table_count);
}

signal_module_status_t SignalWaveformReplay_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
