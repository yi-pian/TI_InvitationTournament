#ifndef SIGNAL_CONTEST_PIPELINE_H
#define SIGNAL_CONTEST_PIPELINE_H

#include <stdint.h>

#include "signal_integration.h"
#include "signal_status.h"

typedef struct {
    uint32_t sample_rate_hz;
    uint32_t sample_count;
    uint8_t adc_bits;
    float adc_vref_v;
    float input_scale;
    float input_offset_v;
    float expected_min_hz;
    float expected_max_hz;
} signal_pipeline_config_t;

typedef struct {
    uint32_t valid_mask;
    signal_meter_result_t basic;
    signal_spectrum_integration_result_t spectrum;
    signal_thd_integration_result_t thd;
    signal_phase_integration_result_t phase;
} signal_pipeline_result_t;

#define SIGNAL_PIPELINE_VALID_BASIC     (1UL << 0)
#define SIGNAL_PIPELINE_VALID_SPECTRUM  (1UL << 1)
#define SIGNAL_PIPELINE_VALID_THD       (1UL << 2)
#define SIGNAL_PIPELINE_VALID_PHASE     (1UL << 3)

void Hardware_Init(void);
signal_result_t SignalPipeline_Init(const signal_pipeline_config_t *config);
signal_result_t SignalPipeline_Acquire(void);
signal_algorithm_status_t SignalPipeline_Process(void);
signal_result_t SignalPipeline_GetResult(signal_pipeline_result_t *result);
void SignalPipeline_OutputResult(const signal_pipeline_result_t *result);

extern volatile signal_pipeline_result_t g_signal_pipeline_last_output;

#endif
