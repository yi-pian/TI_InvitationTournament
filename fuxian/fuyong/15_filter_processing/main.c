/* 15_filter_processing: one-frame voltage filtering teaching example. */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "arm_math.h"
#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_median_filter.h"
#include "signal_hampel.h"

static uint16_t adc_samples[SIGNAL_SAMPLE_COUNT];
static uint16_t adc_unused_samples[SIGNAL_SAMPLE_COUNT];
/* [INPUT] voltage_samples[]: float physical voltage, V. */
static float voltage_samples[SIGNAL_SAMPLE_COUNT];
/* [OUTPUT] filtered_samples[]: float physical voltage, V. */
static float filtered_samples[SIGNAL_SAMPLE_COUNT];
static float workspace[SIGNAL_SAMPLE_COUNT];
static float fir_state[SIGNAL_SAMPLE_COUNT + 4U];
static float iir_state[4U];
static uint32_t outlier_count;
typedef enum { FILTER_MOVING_AVERAGE, FILTER_MEDIAN, FILTER_HAMPEL, FILTER_FIR, FILTER_IIR } filter_mode_t;
static const filter_mode_t s_filter_mode = FILTER_HAMPEL;
static const signal_dual_adc_config_t s_adc_config = { SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U };

static bool AcquireADCFrame(void)
{ if (SignalDualADC_Start(adc_samples, adc_unused_samples, SIGNAL_SAMPLE_COUNT) != SIGNAL_RESULT_OK) return false;
  while (!SignalDualADC_IsFinished()) { __WFI(); } return true; }

/* [COPY START: FILTER_CONVERT]
 * [INPUT] adc_samples[] uint16_t ADC code. [OUTPUT] voltage_samples[] V.
 * Dependency: signal_config.h. Single-frame unique: YES. */
static void ConvertADCToVoltage(void)
{ uint32_t i; for (i = 0U; i < SIGNAL_SAMPLE_COUNT; ++i)
    voltage_samples[i] = (float)adc_samples[i] * SIGNAL_ADC_VREF_V / 4095.0F; }
/* [COPY END: FILTER_CONVERT] */

/* [COPY START: MOVING_AVERAGE]
 * [INPUT] voltage_samples[] V. [OUTPUT] filtered_samples[] V.
 * Dependency: none. Single-frame unique: YES. */
static void ApplyMovingAverage(void)
{ uint32_t i, begin; float sum = 0.0F; for (i = 0U; i < SIGNAL_SAMPLE_COUNT; ++i) {
    sum += voltage_samples[i]; if (i >= 5U) sum -= voltage_samples[i - 5U];
    begin = (i < 4U) ? 0U : i - 4U; filtered_samples[i] = sum / (float)(i - begin + 1U); } }
/* [COPY END: MOVING_AVERAGE] */

/* [COPY START: MEDIAN_HAMPEL]
 * [INPUT] voltage_samples[] V. [OUTPUT] filtered_samples[] V, outlier_count.
 * Dependency: signal_median_filter / signal_hampel; workspace[]. Single-frame unique: YES. */
static bool ApplyMedianOrHampel(bool hampel)
{ if (!hampel) return SignalMedianFilter_Process(voltage_samples, filtered_samples, SIGNAL_SAMPLE_COUNT, 5U, workspace, SIGNAL_SAMPLE_COUNT) == SIGNAL_ALGORITHM_OK;
  { signal_hampel_config_t cfg = {5U, 3.0F, 0.001F}; signal_hampel_result_t r;
    if (SignalHampel_Process(voltage_samples, filtered_samples, SIGNAL_SAMPLE_COUNT, &cfg, workspace, SIGNAL_SAMPLE_COUNT, &r) != SIGNAL_ALGORITHM_OK) return false;
    outlier_count = r.replaced_count; return true; } }
/* [COPY END: MEDIAN_HAMPEL] */

/* [COPY START: FIR_IIR]
 * [INPUT] voltage_samples[] V. [OUTPUT] filtered_samples[] V.
 * Dependency: CMSIS-DSP. Single-frame unique: YES; FIR state is reset per frame. */
static void ApplyFIRorIIR(bool iir)
{ if (!iir) { static const float c[5] = {0.0625F,0.25F,0.375F,0.25F,0.0625F}; arm_fir_instance_f32 s;
    memset(fir_state, 0, sizeof(fir_state)); arm_fir_init_f32(&s, 5U, c, fir_state, SIGNAL_SAMPLE_COUNT); arm_fir_f32(&s, voltage_samples, filtered_samples, SIGNAL_SAMPLE_COUNT);
  } else { static const float c[5] = {0.067455F,0.134911F,0.067455F,-1.14298F,0.412802F}; arm_biquad_casd_df1_inst_f32 s;
    memset(iir_state, 0, sizeof(iir_state)); arm_biquad_cascade_df1_init_f32(&s, 1U, c, iir_state); arm_biquad_cascade_df1_f32(&s, voltage_samples, filtered_samples, SIGNAL_SAMPLE_COUNT); } }
/* [COPY END: FIR_IIR] */

static bool Filter_Process(void)
{ outlier_count = 0U; switch (s_filter_mode) { case FILTER_MOVING_AVERAGE: ApplyMovingAverage(); return true;
  case FILTER_MEDIAN: return ApplyMedianOrHampel(false); case FILTER_HAMPEL: return ApplyMedianOrHampel(true);
  case FILTER_FIR: ApplyFIRorIIR(false); return true; default: ApplyFIRorIIR(true); return true; } }

int main(void)
{ SYSCFG_DL_init(); if (SignalDualADC_Init(&s_adc_config) != SIGNAL_RESULT_OK) while (true) { }
  while (true) if (AcquireADCFrame()) { ConvertADCToVoltage(); (void)Filter_Process(); } }
