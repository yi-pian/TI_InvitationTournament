#include "signal_pipeline.h"

#include <stdbool.h>
#include <stddef.h>

#include "signal_config.h"
#include "signal_features.h"
#include "signal_dual_adc_platform.h"
#include "ti_msp_dl_config.h"

static signal_pipeline_config_t g_config;
static signal_pipeline_result_t g_result;
static bool g_initialized;
static bool g_acquired;

static uint16_t g_raw_a[SAMPLE_COUNT];
static uint16_t g_raw_b[SAMPLE_COUNT];
static float g_voltage_a[SAMPLE_COUNT];

#if SIGNAL_CONTEST_ENABLE_BASIC
static signal_zero_cross_event_t g_events[SAMPLE_COUNT / 2U + 1U];
static float g_positions[SAMPLE_COUNT / 2U + 1U];
#endif

#if SIGNAL_CONTEST_ENABLE_SPECTRUM || SIGNAL_CONTEST_ENABLE_THD || \
    SIGNAL_CONTEST_ENABLE_PHASE
static signal_complex_f32_t g_fft_a[SAMPLE_COUNT];
#endif

#if SIGNAL_CONTEST_ENABLE_SPECTRUM || SIGNAL_CONTEST_ENABLE_THD
static float g_magnitude[SAMPLE_COUNT / 2U + 1U];
#endif

#if SIGNAL_CONTEST_ENABLE_PHASE
static float g_voltage_b[SAMPLE_COUNT];
static signal_complex_f32_t g_fft_b[SAMPLE_COUNT];
static float g_correlation[2U * SIGNAL_MAX_PHASE_LAG + 1U];
#endif

volatile signal_pipeline_result_t g_signal_pipeline_last_output;

void Hardware_Init(void)
{
    SYSCFG_DL_init();
}

signal_result_t SignalPipeline_Init(const signal_pipeline_config_t *config)
{
    signal_result_t status;
    if ((config == NULL) || (config->sample_count != SAMPLE_COUNT) ||
        (config->sample_rate_hz == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    g_config = *config;
    status = SignalDualADCPlatform_Init(
        config->sample_rate_hz, CPUCLK_FREQ);
    if (status != SIGNAL_RESULT_OK) { return status; }
    g_initialized = true;
    g_acquired = false;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalPipeline_Acquire(void)
{
    signal_result_t status;
    if (!g_initialized) { return SIGNAL_RESULT_NOT_INITIALIZED; }
    status = SignalDualADCPlatform_Start(g_raw_a, g_raw_b, SAMPLE_COUNT);
    if (status != SIGNAL_RESULT_OK) { return status; }
    while (!SignalDualADCPlatform_IsFinished()) { __WFI(); }
    g_acquired = true;
    return SIGNAL_RESULT_OK;
}

signal_algorithm_status_t SignalPipeline_Process(void)
{
    signal_algorithm_status_t status;
    float actual_rate;
    if (!g_acquired) { return SIGNAL_ALGORITHM_INVALID_ARGUMENT; }
    g_result = (signal_pipeline_result_t) {0};
    actual_rate = (float) SignalDualADCPlatform_GetConfiguredRate();

#if SIGNAL_CONTEST_ENABLE_BASIC
    status = SignalIntegration_SignalMeter(g_raw_a, SAMPLE_COUNT,
        g_config.adc_bits, g_config.adc_vref_v, g_config.input_scale,
        g_config.input_offset_v, actual_rate,
        SIGNAL_METER_MEASURE_DC | SIGNAL_METER_MEASURE_MIN_MAX |
        SIGNAL_METER_MEASURE_VPP | SIGNAL_METER_MEASURE_RMS |
        SIGNAL_METER_MEASURE_AC_RMS | SIGNAL_METER_MEASURE_FREQUENCY,
        SIGNAL_ZERO_CROSS_HYSTERESIS, g_voltage_a, SAMPLE_COUNT,
        g_events, SAMPLE_COUNT / 2U + 1U,
        g_positions, SAMPLE_COUNT / 2U + 1U, &g_result.basic);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    g_result.valid_mask |= SIGNAL_PIPELINE_VALID_BASIC;
#endif

#if SIGNAL_CONTEST_ENABLE_SPECTRUM
    status = SignalIntegration_RawToVoltage(g_raw_a, SAMPLE_COUNT,
        g_config.adc_bits, g_config.adc_vref_v, g_config.input_scale,
        g_config.input_offset_v, g_voltage_a, SAMPLE_COUNT);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    status = SignalIntegration_Spectrum(g_voltage_a, SAMPLE_COUNT,
        actual_rate, g_config.expected_min_hz, g_config.expected_max_hz,
        g_fft_a, SAMPLE_COUNT, g_magnitude, SAMPLE_COUNT / 2U + 1U,
        SIGNAL_PEAK_COUNT, &g_result.spectrum);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    g_result.valid_mask |= SIGNAL_PIPELINE_VALID_SPECTRUM;
#endif

#if SIGNAL_CONTEST_ENABLE_THD
    status = SignalIntegration_RawToVoltage(g_raw_a, SAMPLE_COUNT,
        g_config.adc_bits, g_config.adc_vref_v, g_config.input_scale,
        g_config.input_offset_v, g_voltage_a, SAMPLE_COUNT);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    status = SignalIntegration_THD(g_voltage_a, SAMPLE_COUNT, actual_rate,
        g_config.expected_min_hz, g_config.expected_max_hz,
        SIGNAL_HARMONIC_RADIUS, g_fft_a, SAMPLE_COUNT, g_magnitude,
        SAMPLE_COUNT / 2U + 1U, &g_result.thd);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    g_result.valid_mask |= SIGNAL_PIPELINE_VALID_THD;
#endif

#if SIGNAL_CONTEST_ENABLE_PHASE
    status = SignalIntegration_RawToVoltage(g_raw_a, SAMPLE_COUNT,
        g_config.adc_bits, g_config.adc_vref_v, g_config.input_scale,
        g_config.input_offset_v, g_voltage_a, SAMPLE_COUNT);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    status = SignalIntegration_RawToVoltage(g_raw_b, SAMPLE_COUNT,
        g_config.adc_bits, g_config.adc_vref_v, g_config.input_scale,
        g_config.input_offset_v, g_voltage_b, SAMPLE_COUNT);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    status = SignalIntegration_DualPhase(g_voltage_a, g_voltage_b,
        SAMPLE_COUNT, actual_rate, SIGNAL_PHASE_FREQUENCY_HZ,
        SIGNAL_MAX_PHASE_LAG, g_fft_a, g_fft_b, SAMPLE_COUNT,
        g_correlation, 2U * SIGNAL_MAX_PHASE_LAG + 1U, &g_result.phase);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    g_result.valid_mask |= SIGNAL_PIPELINE_VALID_PHASE;
#endif

    g_acquired = false;
    return SIGNAL_ALGORITHM_OK;
}

signal_result_t SignalPipeline_GetResult(signal_pipeline_result_t *result)
{
    if (result == NULL) { return SIGNAL_RESULT_INVALID_ARGUMENT; }
    *result = g_result;
    return SIGNAL_RESULT_OK;
}

void SignalPipeline_OutputResult(const signal_pipeline_result_t *result)
{
    if (result != NULL) {
        g_signal_pipeline_last_output = *result;
    }
    /* Add UART/UI formatting here; keep it outside acquisition and DSP. */
}
