#include "signal_dds.h"

#include <stddef.h>

static int SignalDDS_IsPowerOfTwo(size_t value)
{
    return (value != 0U) && ((value & (value - 1U)) == 0U);
}

signal_result_t SignalDDS_SetFrequency(signal_dds_t *dds,
    float output_frequency_hz, float update_rate_hz)
{
    double tuning;
    if ((dds == NULL) || !(update_rate_hz > 0.0f) ||
        (output_frequency_hz < 0.0f) ||
        (output_frequency_hz >= update_rate_hz / 2.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    tuning = (double) output_frequency_hz * 4294967296.0 /
        (double) update_rate_hz;
    dds->tuning_word = (uint32_t) (tuning + 0.5);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalDDS_Init(signal_dds_t *dds, const uint16_t *table,
    size_t table_count, float output_frequency_hz, float update_rate_hz,
    uint32_t initial_phase)
{
    if ((dds == NULL) || (table == NULL) || (table_count < 2U) ||
        !SignalDDS_IsPowerOfTwo(table_count) || (table_count > 65536U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    dds->table = table;
    dds->table_count = table_count;
    dds->phase_accumulator = initial_phase;
    return SignalDDS_SetFrequency(dds, output_frequency_hz, update_rate_hz);
}

uint16_t SignalDDS_Next(signal_dds_t *dds)
{
    uint64_t scaled_index;
    size_t index;
    if ((dds == NULL) || (dds->table == NULL) || (dds->table_count == 0U)) {
        return 0U;
    }
    scaled_index = (uint64_t) dds->phase_accumulator * dds->table_count;
    index = (size_t) (scaled_index >> 32U);
    dds->phase_accumulator += dds->tuning_word;
    return dds->table[index];
}

signal_result_t SignalDDS_Fill(signal_dds_t *dds, uint16_t *output,
    size_t count)
{
    size_t index;
    if ((dds == NULL) || (output == NULL) || (count == 0U) ||
        (dds->table == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    for (index = 0U; index < count; ++index) { output[index] = SignalDDS_Next(dds); }
    return SIGNAL_RESULT_OK;
}

float SignalDDS_GetConfiguredFrequency(const signal_dds_t *dds,
    float update_rate_hz)
{
    if ((dds == NULL) || !(update_rate_hz > 0.0f)) { return 0.0f; }
    return (float) ((double) dds->tuning_word * (double) update_rate_hz /
        4294967296.0);
}

signal_module_status_t SignalDDS_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
