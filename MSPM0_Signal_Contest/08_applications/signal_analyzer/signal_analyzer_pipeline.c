#include "signal_analyzer_pipeline.h"

#include <stddef.h>

#include "signal_config.h"

static float g_voltage_a[SIGNAL_SAMPLE_COUNT];

#if SIGNAL_FEATURE_PHASE
static float g_voltage_b[SIGNAL_SAMPLE_COUNT];
static signal_complex_f32_t g_fft_b[SIGNAL_SAMPLE_COUNT];
static float g_correlation[2U * SIGNAL_MAX_CORRELATION_LAG + 1U];
#endif

#if SIGNAL_FEATURE_SPECTRUM || SIGNAL_FEATURE_THD || SIGNAL_FEATURE_PHASE
static signal_complex_f32_t g_fft_a[SIGNAL_SAMPLE_COUNT];
#endif

#if SIGNAL_FEATURE_SPECTRUM || SIGNAL_FEATURE_THD
static float g_magnitude[SIGNAL_SAMPLE_COUNT / 2U + 1U];
#endif

#if SIGNAL_FEATURE_FREQUENCY
static signal_zero_cross_event_t g_events[SIGNAL_SAMPLE_COUNT / 2U + 1U];
static float g_crossing_positions[SIGNAL_SAMPLE_COUNT / 2U + 1U];
#endif

#if SIGNAL_FEATURE_DC || SIGNAL_FEATURE_MIN_MAX || SIGNAL_FEATURE_VPP || \
    SIGNAL_FEATURE_RMS || SIGNAL_FEATURE_AC_RMS || SIGNAL_FEATURE_FREQUENCY
static uint32_t SignalAnalyzer_MeterMask(void)
{
    uint32_t mask = 0U;
#if SIGNAL_FEATURE_DC
    mask |= SIGNAL_METER_MEASURE_DC;
#endif
#if SIGNAL_FEATURE_MIN_MAX
    mask |= SIGNAL_METER_MEASURE_MIN_MAX;
#endif
#if SIGNAL_FEATURE_VPP
    mask |= SIGNAL_METER_MEASURE_VPP;
#endif
#if SIGNAL_FEATURE_RMS
    mask |= SIGNAL_METER_MEASURE_RMS;
#endif
#if SIGNAL_FEATURE_AC_RMS
    mask |= SIGNAL_METER_MEASURE_AC_RMS;
#endif
#if SIGNAL_FEATURE_FREQUENCY
    mask |= SIGNAL_METER_MEASURE_FREQUENCY;
#endif
    return mask;
}
#endif

signal_algorithm_status_t SignalAnalyzerPipeline_Process(
    const uint16_t *raw_a, const uint16_t *raw_b, uint32_t sample_rate_hz,
    signal_analyzer_pipeline_result_t *result)
{
    signal_algorithm_status_t status;

    if ((raw_a == NULL) || (result == NULL) || (sample_rate_hz == 0U)) {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    *result = (signal_analyzer_pipeline_result_t) {0};

#if SIGNAL_FEATURE_DC || SIGNAL_FEATURE_MIN_MAX || SIGNAL_FEATURE_VPP || \
    SIGNAL_FEATURE_RMS || SIGNAL_FEATURE_AC_RMS || SIGNAL_FEATURE_FREQUENCY
    status = SignalIntegration_SignalMeter(raw_a, SIGNAL_SAMPLE_COUNT,
        SIGNAL_ADC_BITS, SIGNAL_ADC_A_VREF_V, SIGNAL_INPUT_A_SCALE,
        SIGNAL_INPUT_A_OFFSET_V, (float) sample_rate_hz,
        SignalAnalyzer_MeterMask(), SIGNAL_ZERO_CROSS_HYSTERESIS_V,
        g_voltage_a, SIGNAL_SAMPLE_COUNT,
#if SIGNAL_FEATURE_FREQUENCY
        g_events, SIGNAL_SAMPLE_COUNT / 2U + 1U,
        g_crossing_positions, SIGNAL_SAMPLE_COUNT / 2U + 1U,
#else
        NULL, 0U, NULL, 0U,
#endif
        &result->meter);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    result->valid_mask |= SIGNAL_ANALYZER_VALID_METER;
#endif

#if SIGNAL_FEATURE_SPECTRUM && !SIGNAL_FEATURE_THD
    status = SignalIntegration_RawToVoltage(raw_a, SIGNAL_SAMPLE_COUNT,
        SIGNAL_ADC_BITS, SIGNAL_ADC_A_VREF_V, SIGNAL_INPUT_A_SCALE,
        SIGNAL_INPUT_A_OFFSET_V, g_voltage_a, SIGNAL_SAMPLE_COUNT);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    status = SignalIntegration_Spectrum(g_voltage_a, SIGNAL_SAMPLE_COUNT,
        (float) sample_rate_hz, SIGNAL_EXPECTED_FREQ_MIN_HZ,
        SIGNAL_EXPECTED_FREQ_MAX_HZ, g_fft_a, SIGNAL_SAMPLE_COUNT,
        g_magnitude, SIGNAL_SAMPLE_COUNT / 2U + 1U,
        SIGNAL_SPECTRUM_PEAK_COUNT, &result->spectrum);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    result->valid_mask |= SIGNAL_ANALYZER_VALID_SPECTRUM;

#if SIGNAL_FEATURE_SNR
    {
        uint32_t center = result->spectrum.main_peak_bin;
        uint32_t start = (center > SIGNAL_SPECTRAL_BAND_RADIUS) ?
            center - SIGNAL_SPECTRAL_BAND_RADIUS : 1U;
        uint32_t end = center + SIGNAL_SPECTRAL_BAND_RADIUS;
        if (end > SIGNAL_SAMPLE_COUNT / 2U) {
            end = SIGNAL_SAMPLE_COUNT / 2U;
        }
        signal_snr_config_t config = {
            start, end,
            1U, SIGNAL_SAMPLE_COUNT / 2U, NULL, 0U
        };
        status = SignalSNR_Process(g_magnitude,
            SIGNAL_SAMPLE_COUNT / 2U + 1U, &config, &result->snr);
        if (status != SIGNAL_ALGORITHM_OK) { return status; }
        result->valid_mask |= SIGNAL_ANALYZER_VALID_SNR;
    }
#endif
#if SIGNAL_FEATURE_SFDR
    {
        uint32_t center = result->spectrum.main_peak_bin;
        uint32_t start = (center > SIGNAL_SPECTRAL_BAND_RADIUS) ?
            center - SIGNAL_SPECTRAL_BAND_RADIUS : 1U;
        uint32_t end = center + SIGNAL_SPECTRAL_BAND_RADIUS;
        if (end > SIGNAL_SAMPLE_COUNT / 2U) {
            end = SIGNAL_SAMPLE_COUNT / 2U;
        }
        signal_sfdr_config_t config = {
            start, end,
            1U, SIGNAL_SAMPLE_COUNT / 2U
        };
        status = SignalSFDR_Process(g_magnitude,
            SIGNAL_SAMPLE_COUNT / 2U + 1U, &config, &result->sfdr);
        if (status != SIGNAL_ALGORITHM_OK) { return status; }
        result->valid_mask |= SIGNAL_ANALYZER_VALID_SFDR;
    }
#endif
#endif

#if SIGNAL_FEATURE_THD
    status = SignalIntegration_RawToVoltage(raw_a, SIGNAL_SAMPLE_COUNT,
        SIGNAL_ADC_BITS, SIGNAL_ADC_A_VREF_V, SIGNAL_INPUT_A_SCALE,
        SIGNAL_INPUT_A_OFFSET_V, g_voltage_a, SIGNAL_SAMPLE_COUNT);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    status = SignalIntegration_THD(g_voltage_a, SIGNAL_SAMPLE_COUNT,
        (float) sample_rate_hz, SIGNAL_EXPECTED_FREQ_MIN_HZ,
        SIGNAL_EXPECTED_FREQ_MAX_HZ, SIGNAL_HARMONIC_BIN_RADIUS,
        g_fft_a, SIGNAL_SAMPLE_COUNT, g_magnitude,
        SIGNAL_SAMPLE_COUNT / 2U + 1U, &result->thd);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    result->valid_mask |= SIGNAL_ANALYZER_VALID_THD;
#endif

#if SIGNAL_FEATURE_PHASE
    if (raw_b == NULL) { return SIGNAL_ALGORITHM_INVALID_ARGUMENT; }
    status = SignalIntegration_RawToVoltage(raw_a, SIGNAL_SAMPLE_COUNT,
        SIGNAL_ADC_BITS, SIGNAL_ADC_A_VREF_V, SIGNAL_INPUT_A_SCALE,
        SIGNAL_INPUT_A_OFFSET_V, g_voltage_a, SIGNAL_SAMPLE_COUNT);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    status = SignalIntegration_RawToVoltage(raw_b, SIGNAL_SAMPLE_COUNT,
        SIGNAL_ADC_BITS, SIGNAL_ADC_B_VREF_V, SIGNAL_INPUT_B_SCALE,
        SIGNAL_INPUT_B_OFFSET_V, g_voltage_b, SIGNAL_SAMPLE_COUNT);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    status = SignalIntegration_DualPhase(g_voltage_a, g_voltage_b,
        SIGNAL_SAMPLE_COUNT, (float) sample_rate_hz,
        SIGNAL_KNOWN_PHASE_FREQUENCY_HZ, SIGNAL_MAX_CORRELATION_LAG,
        g_fft_a, g_fft_b, SIGNAL_SAMPLE_COUNT, g_correlation,
        2U * SIGNAL_MAX_CORRELATION_LAG + 1U, &result->phase);
    if (status != SIGNAL_ALGORITHM_OK) { return status; }
    result->valid_mask |= SIGNAL_ANALYZER_VALID_PHASE;
#endif

    return SIGNAL_ALGORITHM_OK;
}
