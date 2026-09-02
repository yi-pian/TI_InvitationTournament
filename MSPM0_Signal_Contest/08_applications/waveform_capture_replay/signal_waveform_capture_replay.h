#ifndef SIGNAL_WAVEFORM_CAPTURE_REPLAY_H
#define SIGNAL_WAVEFORM_CAPTURE_REPLAY_H

#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

signal_result_t SignalWaveformReplay_Prepare(const uint16_t *captured,
    size_t captured_count, uint16_t captured_min, uint16_t captured_max,
    uint8_t dac_bits, uint16_t *dac_table, size_t dac_table_count);
signal_result_t SignalWaveformReplay_PrepareAutoRange(
    const uint16_t *captured, size_t captured_count, uint8_t dac_bits,
    uint16_t *dac_table, size_t dac_table_count, uint16_t *captured_min,
    uint16_t *captured_max);
signal_module_status_t SignalWaveformReplay_GetModuleStatus(void);

#endif
