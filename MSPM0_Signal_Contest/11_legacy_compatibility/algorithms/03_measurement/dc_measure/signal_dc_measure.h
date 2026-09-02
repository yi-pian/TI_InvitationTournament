#ifndef SIGNAL_DC_MEASURE_H
#define SIGNAL_DC_MEASURE_H

#include <stdint.h>

#include "signal_adc_to_voltage.h"
#include "signal_algorithm_status.h"

typedef struct
{
    float mean_code;
    float dc_voltage_v;
} signal_dc_measure_result_t;

signal_algorithm_status_t SignalDCMeasure_FromRawLinear(
    const uint16_t *raw_codes,
    uint32_t count,
    const signal_adc_to_voltage_config_t *conversion,
    signal_dc_measure_result_t *result);

signal_algorithm_status_t SignalDCMeasure_FromVoltage(
    const float *voltage_v,
    uint32_t count,
    signal_dc_measure_result_t *result);

#endif
