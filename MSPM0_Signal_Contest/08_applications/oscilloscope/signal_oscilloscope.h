#ifndef SIGNAL_OSCILLOSCOPE_H
#define SIGNAL_OSCILLOSCOPE_H

#include <stddef.h>
#include "signal_status.h"

typedef struct {
    float minimum_v;
    float maximum_v;
    float mean_v;
    float vpp_v;
    float total_rms_v;
    float ac_rms_v;
} signal_oscilloscope_measurements_t;

signal_result_t SignalOscilloscope_Analyze(const float *voltage_v,
    size_t count, signal_oscilloscope_measurements_t *measurements);
signal_module_status_t SignalOscilloscope_GetModuleStatus(void);

#endif
