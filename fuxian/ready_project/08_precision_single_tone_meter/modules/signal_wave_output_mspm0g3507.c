#include "signal_wave_output_mspm0g3507.h"

#include "signal_dac_wave_table.h"
#include "signal_dds.h"
#include "signal_sawtooth.h"
#include "signal_sine.h"
#include "signal_square.h"
#include "signal_triangle.h"

typedef enum { WAVE_SINE, WAVE_SQUARE, WAVE_TRIANGLE, WAVE_SAWTOOTH } wave_type_t;

static signal_wave_output_config_t s_config;
static signal_dds_t s_dds;
static signal_wave_output_result_t s_last_result;
static bool s_initialized;
static bool s_has_result;

signal_result_t SignalWaveOutput_Init(
    const signal_wave_output_config_t *config)
{
    signal_result_t result;
    if ((config == NULL) || (config->wave_table == NULL) ||
        (config->output_buffer == NULL) || (config->wave_table_count < 2U) ||
        (config->output_capacity < 2U) || (config->dac_bits == 0U) ||
        !(config->reference_voltage_v > 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    s_initialized = false;
    s_has_result = false;
    s_config = *config;
    result = SignalDACDMA_MSPM0_Init(&s_config.dac_config);
    if (result != SIGNAL_RESULT_OK) return result;
    s_initialized = true;
    return SIGNAL_RESULT_OK;
}

static signal_result_t SignalWaveOutput_Start(wave_type_t type,
    float frequency_hz, float vpp_v, float offset_v, float shape_parameter)
{
    signal_dac_wave_table_t table;
    signal_result_t result;
    uint32_t update_rate_hz;
    uint32_t output_count;
    float vpeak_v;
    float offset_fraction;
    float amplitude_fraction;
    float actual_frequency_hz;

    if (!s_initialized ||
        !(frequency_hz > 0.0f) || !(vpp_v > 0.0f) ||
        !(offset_v >= 0.0f) || !(offset_v <= s_config.reference_voltage_v) ||
        (frequency_hz >= (float)s_config.dac_config.update_rate_hz / 2.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (((type == WAVE_SQUARE) || (type == WAVE_SAWTOOTH)) &&
        ((shape_parameter <= 0.0f) ||
         ((type == WAVE_SQUARE) && (shape_parameter >= 1.0f)) ||
         ((type == WAVE_SAWTOOTH) && (shape_parameter > 1.0f)))) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    vpeak_v = vpp_v * 0.5f;
    if ((offset_v - vpeak_v < 0.0f) ||
        (offset_v + vpeak_v > s_config.reference_voltage_v)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    update_rate_hz = SignalDACDMA_MSPM0_GetConfiguredRate();
    if (update_rate_hz == 0U) return SIGNAL_RESULT_NOT_INITIALIZED;
    output_count = (uint32_t)(((float)update_rate_hz / frequency_hz) + 0.5f);
    if (output_count < 2U) output_count = 2U;
    if (output_count > s_config.output_capacity) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    actual_frequency_hz = (float)update_rate_hz / (float)output_count;
    offset_fraction = offset_v / s_config.reference_voltage_v;
    amplitude_fraction = vpeak_v / s_config.reference_voltage_v;
    table.samples = s_config.wave_table;
    table.count = s_config.wave_table_count;
    table.dac_bits = s_config.dac_bits;

    SignalDACDMA_MSPM0_Stop();
    if (type == WAVE_SINE) {
        result = SignalSine_Generate(&table, offset_fraction, amplitude_fraction, 0.0f);
    } else if (type == WAVE_SQUARE) {
        result = SignalSquare_GenerateWithDuty(&table, offset_fraction,
            amplitude_fraction, shape_parameter, 0.0f);
    } else if (type == WAVE_TRIANGLE) {
        result = SignalTriangle_Generate(&table, offset_fraction, amplitude_fraction, 0.0f);
    } else {
        result = SignalSawtooth_GenerateWithSymmetry(&table, offset_fraction,
            amplitude_fraction, 0.0f, true, shape_parameter);
    }
    if (result != SIGNAL_RESULT_OK) return result;
    result = SignalDACWaveTable_Validate(&table);
    if (result != SIGNAL_RESULT_OK) return result;
    result = SignalDDS_Init(&s_dds, s_config.wave_table,
        s_config.wave_table_count, actual_frequency_hz,
        (float)update_rate_hz, 0U);
    if (result != SIGNAL_RESULT_OK) return result;
    result = SignalDDS_Fill(&s_dds, s_config.output_buffer, output_count);
    if (result != SIGNAL_RESULT_OK) return result;
    result = SignalDACDMA_MSPM0_Start(s_config.output_buffer, output_count, true);
    if (result != SIGNAL_RESULT_OK) return result;
    s_last_result.requested_frequency_hz = frequency_hz;
    s_last_result.actual_frequency_hz = actual_frequency_hz;
    s_last_result.requested_vpp_v = vpp_v;
    s_last_result.actual_vpp_v = vpp_v;
    s_last_result.offset_v = offset_v;
    s_last_result.dma_point_count = output_count;
    s_has_result = true;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalWaveOutput_SineWithOffset(float frequency_hz,
    float vpp_v, float offset_v)
{ return SignalWaveOutput_Start(WAVE_SINE,
    frequency_hz, vpp_v, offset_v, 0.5f); }

signal_result_t SignalWaveOutput_SquareWithOffset(float frequency_hz,
    float vpp_v, float offset_v)
{ return SignalWaveOutput_Start(WAVE_SQUARE,
    frequency_hz, vpp_v, offset_v, 0.5f); }

signal_result_t SignalWaveOutput_SquareWithDuty(float frequency_hz,
    float vpp_v, float offset_v, float duty_fraction)
{ return SignalWaveOutput_Start(WAVE_SQUARE,
    frequency_hz, vpp_v, offset_v, duty_fraction); }

signal_result_t SignalWaveOutput_TriangleWithOffset(float frequency_hz,
    float vpp_v, float offset_v)
{ return SignalWaveOutput_Start(WAVE_TRIANGLE,
    frequency_hz, vpp_v, offset_v, 0.5f); }

signal_result_t SignalWaveOutput_SawtoothWithOffset(float frequency_hz,
    float vpp_v, float offset_v)
{ return SignalWaveOutput_Start(WAVE_SAWTOOTH,
    frequency_hz, vpp_v, offset_v, 1.0f); }

signal_result_t SignalWaveOutput_SawtoothWithSymmetry(float frequency_hz,
    float vpp_v, float offset_v, float symmetry_fraction)
{ return SignalWaveOutput_Start(WAVE_SAWTOOTH,
    frequency_hz, vpp_v, offset_v, symmetry_fraction); }

signal_result_t SignalWaveOutput_GetLastResult(
    signal_wave_output_result_t *result)
{
    if (result == NULL) return SIGNAL_RESULT_INVALID_ARGUMENT;
    if (!s_initialized) return SIGNAL_RESULT_NOT_INITIALIZED;
    if (!s_has_result) return SIGNAL_RESULT_NO_DATA;
    *result = s_last_result;
    return SIGNAL_RESULT_OK;
}

void SignalWaveOutput_Stop(void) { SignalDACDMA_MSPM0_Stop(); }
signal_module_status_t SignalWaveOutput_GetModuleStatus(void)
{ return MODULE_STATUS_BUILD_VERIFIED; }
