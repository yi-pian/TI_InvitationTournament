#ifndef SIGNAL_DAC_H
#define SIGNAL_DAC_H

#include <stdint.h>
#include "signal_status.h"

typedef signal_result_t (*signal_dac_write_fn)(void *context, uint16_t raw);

typedef struct {
    void *context;
    signal_dac_write_fn write;
    uint8_t resolution_bits;
    float reference_voltage_v;
} signal_dac_t;

signal_result_t SignalDAC_VoltageToRaw(float voltage_v, uint8_t bits,
    float reference_voltage_v, uint16_t *raw);
signal_result_t SignalDAC_WriteRaw(const signal_dac_t *dac, uint16_t raw);
signal_module_status_t SignalDAC_GetModuleStatus(void);

#endif
