#ifndef SIGNAL_DDS_GENERATOR_H
#define SIGNAL_DDS_GENERATOR_H

#include <stddef.h>
#include <stdint.h>
#include "signal_dds.h"

signal_result_t SignalDDSGenerator_PrepareSine(signal_dds_t *dds,
    uint16_t *wave_table, size_t table_count, uint8_t dac_bits,
    float offset_fraction, float amplitude_fraction, float output_frequency_hz,
    float update_rate_hz);
signal_module_status_t SignalDDSGenerator_GetModuleStatus(void);

#endif
