#include "signal_dds_generator.h"

#include "signal_dac_wave_table.h"
#include "signal_sine.h"

signal_result_t SignalDDSGenerator_PrepareSine(signal_dds_t *dds,
    uint16_t *wave_table, size_t table_count, uint8_t dac_bits,
    float offset_fraction, float amplitude_fraction, float output_frequency_hz,
    float update_rate_hz)
{
    signal_dac_wave_table_t table = { wave_table, table_count, dac_bits };
    signal_result_t status = SignalSine_Generate(&table, offset_fraction,
        amplitude_fraction, 0.0f);
    if (status != SIGNAL_RESULT_OK) { return status; }
    return SignalDDS_Init(dds, wave_table, table_count, output_frequency_hz,
        update_rate_hz, 0U);
}

signal_module_status_t SignalDDSGenerator_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
