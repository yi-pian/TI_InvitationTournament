#ifndef SIGNAL_ADC_H
#define SIGNAL_ADC_H

#include <stdint.h>
#include "signal_status.h"

typedef struct {
    uint8_t channel;
    uint8_t resolution_bits;
    float reference_voltage_v;
    uint32_t clock_hz;
} signal_adc_config_t;

typedef signal_result_t (*signal_adc_read_fn)(void *context, uint16_t *raw);
typedef signal_result_t (*signal_adc_control_fn)(void *context);

typedef struct {
    void *context;
    signal_adc_read_fn read;
    signal_adc_control_fn enable;
    signal_adc_control_fn disable;
    signal_adc_config_t config;
} signal_adc_t;

signal_result_t SignalADC_ValidateConfig(const signal_adc_config_t *config);
signal_result_t SignalADC_ReadRaw(const signal_adc_t *adc, uint16_t *raw);
signal_module_status_t SignalADC_GetBspModuleStatus(void);

#endif
