#ifndef SIGNAL_ANALYZER_PIPELINE_H
#define SIGNAL_ANALYZER_PIPELINE_H

#include <stdint.h>

#include "signal_integration.h"
#include "signal_sfdr.h"
#include "signal_snr.h"

#define SIGNAL_ANALYZER_VALID_METER     (1UL << 0)
#define SIGNAL_ANALYZER_VALID_SPECTRUM  (1UL << 1)
#define SIGNAL_ANALYZER_VALID_THD       (1UL << 2)
#define SIGNAL_ANALYZER_VALID_SNR       (1UL << 3)
#define SIGNAL_ANALYZER_VALID_SFDR      (1UL << 4)
#define SIGNAL_ANALYZER_VALID_PHASE     (1UL << 5)

typedef struct {
    uint32_t valid_mask;
    signal_meter_result_t meter;
    signal_spectrum_integration_result_t spectrum;
    signal_thd_integration_result_t thd;
    signal_snr_result_t snr;
    signal_sfdr_result_t sfdr;
    signal_phase_integration_result_t phase;
} signal_analyzer_pipeline_result_t;

signal_algorithm_status_t SignalAnalyzerPipeline_Process(
    const uint16_t *raw_a, const uint16_t *raw_b, uint32_t sample_rate_hz,
    signal_analyzer_pipeline_result_t *result);

#endif
