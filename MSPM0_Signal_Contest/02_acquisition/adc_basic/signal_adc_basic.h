#ifndef SIGNAL_ADC_BASIC_H
#define SIGNAL_ADC_BASIC_H

#include <stddef.h>
#include <stdint.h>
#include "signal_adc.h"
#include "signal_status.h"

signal_result_t SignalADCBasic_ReadBlock(const signal_adc_t *adc,
    uint16_t *destination, size_t sample_count);
signal_module_status_t SignalADCBasic_GetModuleStatus(void);

#endif
