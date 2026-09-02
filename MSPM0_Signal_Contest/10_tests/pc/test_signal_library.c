#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "signal_ac_rms.h"
#include "signal_adc_ring_buffer.h"
#include "signal_adc_to_voltage.h"
#include "signal_button.h"
#include "signal_dds.h"
#include "signal_fft.h"
#include "signal_hann.h"
#include "signal_harmonic.h"
#include "signal_integration.h"
#include "signal_latching_button_switch.h"
#include "signal_math.h"
#include "signal_mean.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_minmax.h"
#include "signal_phase.h"
#include "signal_rotary_encoder.h"
#include "signal_sine.h"
#include "signal_sine_fit_3param.h"
#include "signal_thd.h"
#include "signal_tft_ili9341.h"
#include "signal_tft_waveform.h"
#include "signal_timer_capture.h"
#include "signal_vpp.h"
#include "signal_window_gain_correction.h"

#define TEST_N 1024U

static float s_wave[TEST_N];
static float s_windowed[TEST_N];
static signal_complex_f32_t s_fft[TEST_N];
static float s_magnitude[(TEST_N / 2U) + 1U];
static uint16_t s_table[256U];

typedef struct {
    int8_t selected_row;
    uint16_t physical_mask;
    uint32_t delay_calls;
} keypad_mock_t;

typedef struct {
    bool cs;
    bool dc;
    bool reset;
    bool backlight;
    size_t writes;
    size_t bytes;
    size_t commands;
    uint8_t first_command;
    uint8_t last_command;
} tft_mock_t;

typedef struct {
    bool value;
    size_t reads;
} digital_input_mock_t;

typedef struct {
    bool a_high;
    bool b_high;
    bool button_high;
} rotary_encoder_mock_t;

static signal_result_t MockRotaryA(void *context, bool *high)
{
    rotary_encoder_mock_t *mock = (rotary_encoder_mock_t *)context;
    if ((mock == NULL) || (high == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *high = mock->a_high;
    return SIGNAL_RESULT_OK;
}

static signal_result_t MockRotaryB(void *context, bool *high)
{
    rotary_encoder_mock_t *mock = (rotary_encoder_mock_t *)context;
    if ((mock == NULL) || (high == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *high = mock->b_high;
    return SIGNAL_RESULT_OK;
}

static signal_result_t MockRotaryButton(void *context, bool *high)
{
    rotary_encoder_mock_t *mock = (rotary_encoder_mock_t *)context;
    if ((mock == NULL) || (high == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *high = mock->button_high;
    return SIGNAL_RESULT_OK;
}

static signal_result_t MockDigitalInput(void *context, bool *active)
{
    digital_input_mock_t *mock = (digital_input_mock_t *)context;
    if ((mock == NULL) || (active == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *active = mock->value;
    ++mock->reads;
    return SIGNAL_RESULT_OK;
}

static signal_result_t MockKeypadDriveRow(void *context, uint8_t row,
    bool active)
{
    keypad_mock_t *mock = (keypad_mock_t *)context;
    if ((mock == NULL) || (row >= SIGNAL_MATRIX_KEYPAD_4X4_ROWS)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (active) {
        mock->selected_row = (int8_t)row;
    } else if (mock->selected_row == (int8_t)row) {
        mock->selected_row = -1;
    }
    return SIGNAL_RESULT_OK;
}

static signal_result_t MockKeypadReadColumn(void *context, uint8_t column,
    bool *active)
{
    keypad_mock_t *mock = (keypad_mock_t *)context;
    uint8_t index;
    if ((mock == NULL) || (active == NULL) ||
        (column >= SIGNAL_MATRIX_KEYPAD_4X4_COLUMNS) ||
        (mock->selected_row < 0)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    index = (uint8_t)((uint8_t)mock->selected_row * 4U + column);
    *active = (mock->physical_mask &
        (uint16_t)(UINT16_C(1) << index)) != 0U;
    return SIGNAL_RESULT_OK;
}

static void MockKeypadDelay(void *context, uint32_t microseconds)
{
    keypad_mock_t *mock = (keypad_mock_t *)context;
    assert(microseconds == 5U);
    ++mock->delay_calls;
}

static int MockTFTWrite(void *context, const uint8_t *data, size_t length)
{
    tft_mock_t *mock = (tft_mock_t *)context;
    if ((mock == NULL) || ((data == NULL) && (length != 0U))) {
        return -1;
    }
    if ((!mock->dc) && (length == 1U)) {
        if (mock->commands == 0U) {
            mock->first_command = data[0];
        }
        mock->last_command = data[0];
        ++mock->commands;
    }
    ++mock->writes;
    mock->bytes += length;
    return 0;
}

static void MockTFTSetCS(void *context, bool high)
{
    ((tft_mock_t *)context)->cs = high;
}

static void MockTFTSetDC(void *context, bool high)
{
    ((tft_mock_t *)context)->dc = high;
}

static void MockTFTSetReset(void *context, bool high)
{
    ((tft_mock_t *)context)->reset = high;
}

static void MockTFTSetBacklight(void *context, bool high)
{
    ((tft_mock_t *)context)->backlight = high;
}

static void MockTFTDelay(void *context, uint32_t milliseconds)
{
    (void)context;
    assert(milliseconds > 0U);
}

static int NearlyEqual(float actual, float expected, float tolerance)
{
    return fabsf(actual - expected) <= tolerance;
}

static void TestMeasurements(void)
{
    const uint16_t raw[] = { 1000U, 2000U, 3000U, 4000U };
    const signal_adc_to_voltage_config_t config = {
        4095U, 3.3f, 1.0f, 0.0f
    };
    float voltage[4U];
    signal_mean_result_t mean;
    signal_minmax_result_t minmax;
    signal_vpp_result_t vpp;
    assert(SignalADCToVoltage_Process(raw, voltage, 4U, &config) ==
        SIGNAL_ALGORITHM_OK);
    assert(SignalMean_Process(voltage, 4U, &mean) == SIGNAL_ALGORITHM_OK);
    assert(NearlyEqual(mean.mean_value, 2500.0f * 3.3f / 4095.0f,
        0.001f));
    assert(SignalMinMax_Process(voltage, 4U, &minmax) ==
        SIGNAL_ALGORITHM_OK);
    assert((minmax.min_index == 0U) && (minmax.max_index == 3U));
    assert(SignalVPP_Process(voltage, 4U, &vpp) == SIGNAL_ALGORITHM_OK);
    assert(NearlyEqual(vpp.amplitude_vpp, 3000.0f * 3.3f / 4095.0f,
        0.001f));
}

static void FillSine(float amplitude, float dc, size_t bin)
{
    size_t index;
    for (index = 0U; index < TEST_N; ++index) {
        s_wave[index] = dc + amplitude * sinf(SIGNAL_TWO_PI_F *
            (float) bin * (float) index / (float) TEST_N);
    }
}

static void TestTimeAndFrequency(void)
{
    signal_ac_rms_result_t ac_rms;
    signal_zero_cross_event_t events[32U];
    float crossing_positions[32U];
    float frequency_hz;
    const float sample_rate_hz = 100000.0f;
    const size_t bin = 16U;
    FillSine(2.0f, 1.0f, bin);
    assert(SignalACRMS_Process(s_wave, TEST_N, &ac_rms) ==
        SIGNAL_ALGORITHM_OK);
    assert(NearlyEqual(ac_rms.mean_voltage_v, 1.0f, 0.001f));
    assert(NearlyEqual(ac_rms.ac_rms_v, sqrtf(2.0f), 0.002f));
    assert(SignalIntegration_FrequencyTime(s_wave, TEST_N, sample_rate_hz,
        0.01f, events, 32U, crossing_positions, 32U, &frequency_hz) ==
        SIGNAL_ALGORITHM_OK);
    assert(NearlyEqual(frequency_hz,
        sample_rate_hz * (float) bin / (float) TEST_N, 0.05f));
}

static void TestSpectrum(void)
{
    signal_spectrum_integration_result_t result;
    const float sample_rate_hz = 100000.0f;
    const size_t bin = 37U;
    FillSine(1.0f, 0.0f, bin);
    assert(SignalIntegration_Spectrum(s_wave, TEST_N, sample_rate_hz,
        100.0f, 49000.0f, s_fft, TEST_N, s_magnitude,
        (TEST_N / 2U) + 1U, 3U, &result) == SIGNAL_ALGORITHM_OK);
    assert(result.main_peak_bin == bin);
    assert(NearlyEqual(result.frequency_hz,
        sample_rate_hz * (float) bin / (float) TEST_N, 0.5f));
}

static void TestSineFit(void)
{
    signal_sine_fit_3param_result_t fit;
    signal_sine_fit_3param_config_t config;
    const float sample_rate_hz = 48000.0f;
    const float frequency_hz = 1500.0f;
    size_t index;
    for (index = 0U; index < TEST_N; ++index) {
        s_wave[index] = 0.75f + 1.25f * sinf(SIGNAL_TWO_PI_F *
            frequency_hz * (float) index / sample_rate_hz + 0.3f);
    }
    config.frequency_hz = frequency_hz;
    config.sample_rate_hz = sample_rate_hz;
    assert(SignalSineFit3Param_Process(s_wave, TEST_N, &config, &fit) ==
        SIGNAL_ALGORITHM_OK);
    assert(NearlyEqual(fit.amplitude_peak_v, 1.25f, 0.002f));
    assert(NearlyEqual(fit.dc_offset_v, 0.75f, 0.002f));
    assert(fit.residual_rms_v < 0.001f);
}

static void TestTHDAndWindowCorrection(void)
{
    signal_harmonic_result_t harmonics = {0};
    signal_thd_result_t thd;
    signal_window_result_t window;
    float ones[8U] = {1.0f, 1.0f, 1.0f, 1.0f,
                      1.0f, 1.0f, 1.0f, 1.0f};
    float raw_magnitude[5U] = {0.0f, 1.75f, 0.0f, 0.0f, 0.0f};
    float amplitude[5U];

    harmonics.first_order = 1U;
    harmonics.last_order = 3U;
    harmonics.items[1U].energy = 1.0f;
    harmonics.items[2U].energy = 0.01f;
    harmonics.items[3U].energy = 0.0025f;
    assert(SignalTHD_Process(&harmonics, &thd) == SIGNAL_ALGORITHM_OK);
    assert(NearlyEqual(thd.thd_ratio, sqrtf(0.0125f), 0.00001f));
    assert(NearlyEqual(thd.thd_percent, 11.18034f, 0.001f));

    assert(SignalHann_Apply(ones, s_windowed, 8U, &window) ==
        SIGNAL_ALGORITHM_OK);
    assert(NearlyEqual(window.coherent_gain, 0.4375f, 0.00001f));
    assert(SignalWindowGainCorrection_Apply(raw_magnitude, amplitude,
        5U, 8U, window.coherent_gain) == SIGNAL_ALGORITHM_OK);
    assert(NearlyEqual(amplitude[1U], 1.0f, 0.00001f));
}

static void TestDDSAndBuffers(void)
{
    signal_dac_wave_table_t table = { s_table, 256U, 12U };
    signal_dds_t dds;
    uint16_t ring_storage[8U];
    signal_adc_ring_buffer_t ring;
    uint16_t sample;
    assert(SignalSine_Generate(&table, 0.5f, 0.45f, 0.0f) ==
        SIGNAL_RESULT_OK);
    assert(SignalDDS_Init(&dds, s_table, 256U, 1000.0f, 100000.0f, 0U) ==
        SIGNAL_RESULT_OK);
    assert(NearlyEqual(SignalDDS_GetConfiguredFrequency(&dds, 100000.0f),
        1000.0f, 0.01f));
    assert(SignalADCRing_Init(&ring, ring_storage, 8U) == SIGNAL_RESULT_OK);
    assert(SignalADCRing_Push(&ring, 1234U) == SIGNAL_RESULT_OK);
    assert(SignalADCRing_Pop(&ring, &sample) == SIGNAL_RESULT_OK);
    assert(sample == 1234U);
}

static void TestCaptureMath(void)
{
    const uint32_t timestamps[] = { 65530U, 94U, 194U, 294U };
    signal_timer_capture_config_t config = { 1000000U, 65536U };
    float mean_ticks;
    float frequency_hz;
    signal_phase_result_t phase;
    assert(SignalTimerCapture_MeanPeriod(timestamps, 4U, &config,
        &mean_ticks, &frequency_hz) == SIGNAL_RESULT_OK);
    assert(NearlyEqual(mean_ticks, 100.0f, 0.001f));
    assert(NearlyEqual(frequency_hz, 10000.0f, 0.1f));
    assert(SignalPhase_FromCorrelationLag(25.0f, 100.0f, &phase) ==
        SIGNAL_ALGORITHM_OK);
    assert(NearlyEqual(phase.phase_difference_deg, -90.0f, 0.001f));
}

static void TestMatrixKeypad4x4(void)
{
    keypad_mock_t mock = { -1, 0U, 0U };
    signal_matrix_keypad_4x4_t keypad;
    signal_matrix_keypad_4x4_event_t event;
    const signal_matrix_keypad_4x4_config_t config = {
        &mock,
        MockKeypadDriveRow,
        MockKeypadReadColumn,
        MockKeypadDelay,
        5U,
        3U,
        NULL
    };
    char symbol;
    uint8_t index;

    assert(SignalMatrixKeypad4x4_Init(&keypad, &config) ==
        SIGNAL_RESULT_OK);
    mock.physical_mask = (uint16_t)(UINT16_C(1) << 6U);
    assert(SignalMatrixKeypad4x4_Scan(&keypad, &event) == SIGNAL_RESULT_OK);
    assert(event.pressed_mask == 0U);
    assert(SignalMatrixKeypad4x4_Scan(&keypad, &event) == SIGNAL_RESULT_OK);
    assert(event.pressed_mask == 0U);
    assert(SignalMatrixKeypad4x4_Scan(&keypad, &event) == SIGNAL_RESULT_OK);
    assert(event.pressed_mask == (uint16_t)(UINT16_C(1) << 6U));
    assert(SignalMatrixKeypad4x4_GetFirstPressed(&keypad, &symbol, &index) ==
        SIGNAL_RESULT_OK);
    assert((symbol == '6') && (index == 6U));

    mock.physical_mask = 0U;
    assert(SignalMatrixKeypad4x4_Scan(&keypad, &event) == SIGNAL_RESULT_OK);
    assert(SignalMatrixKeypad4x4_Scan(&keypad, &event) == SIGNAL_RESULT_OK);
    assert(SignalMatrixKeypad4x4_Scan(&keypad, &event) == SIGNAL_RESULT_OK);
    assert(event.released_mask == (uint16_t)(UINT16_C(1) << 6U));
    assert(SignalMatrixKeypad4x4_GetFirstPressed(&keypad, &symbol, &index) ==
        SIGNAL_RESULT_NO_DATA);

    mock.physical_mask = (uint16_t)((UINT16_C(1) << 0U) |
                                    (UINT16_C(1) << 5U));
    assert(SignalMatrixKeypad4x4_Scan(&keypad, &event) == SIGNAL_RESULT_OK);
    assert(event.ghost_possible);
    assert(mock.delay_calls == 28U);
    assert(SignalMatrixKeypad4x4_GetModuleStatus() ==
        MODULE_STATUS_BUILD_VERIFIED);
}

static void TestTFTILI9341(void)
{
    tft_mock_t mock = { false, false, false, false, 0U, 0U, 0U, 0U, 0U };
    tft_ili9341_t tft;
    uint8_t font_width;
    uint8_t font_height;
    size_t bytes_before_text;
    const uint8_t custom_glyph[] = {UINT8_C(0xAA), UINT8_C(0x55)};
    const tft_ili9341_config_t config = {
        &mock,
        MockTFTWrite,
        MockTFTSetCS,
        MockTFTSetDC,
        MockTFTSetReset,
        MockTFTSetBacklight,
        MockTFTDelay,
        NULL,
        NULL
    };

    assert(TFT_ILI9341_Init(&tft, &config, TFT_ILI9341_ROTATION_90) ==
        SIGNAL_RESULT_OK);
    assert((TFT_ILI9341_GetWidth(&tft) == 320U) &&
           (TFT_ILI9341_GetHeight(&tft) == 240U));
    assert(mock.cs && mock.dc && mock.reset && mock.backlight);
    assert((mock.first_command == UINT8_C(0x01)) &&
           (mock.last_command == UINT8_C(0x36)));
    assert(TFT_ILI9341_FillRect(&tft, -2, -2, 4, 4,
        TFT_ILI9341_RED) == SIGNAL_RESULT_OK);
    assert(mock.bytes > 100U);

    assert(TFT_ILI9341_GetFontMetrics(TFT_ILI9341_FONT_8X16,
        &font_width, &font_height) == SIGNAL_RESULT_OK);
    assert((font_width == 8U) && (font_height == 16U));
    assert(TFT_ILI9341_GetFontMetrics((tft_ili9341_font_t)99,
        &font_width, &font_height) == SIGNAL_RESULT_INVALID_ARGUMENT);

    bytes_before_text = mock.bytes;
    assert(TFT_ILI9341_DrawChar(&tft, 4, 4, 'A',
        TFT_ILI9341_FONT_8X16, TFT_ILI9341_WHITE, TFT_ILI9341_BLACK,
        false) == SIGNAL_RESULT_OK);
    assert(mock.bytes >= (bytes_before_text + 8U * 16U * 2U));
    assert(TFT_ILI9341_DrawString(&tft, 0, 20, "VPP=1.25V\nRMS=0.50V",
        TFT_ILI9341_FONT_6X12, TFT_ILI9341_GREEN, TFT_ILI9341_BLACK,
        false, true) == SIGNAL_RESULT_OK);
    assert(TFT_ILI9341_DrawInt32(&tft, 0, 50, INT32_MIN,
        TFT_ILI9341_FONT_6X12, TFT_ILI9341_YELLOW, TFT_ILI9341_BLACK,
        false) == SIGNAL_RESULT_OK);
    assert(TFT_ILI9341_DrawFloat(&tft, 0, 65, -12.345F, 2U,
        TFT_ILI9341_FONT_8X16, TFT_ILI9341_CYAN, TFT_ILI9341_BLACK,
        false) == SIGNAL_RESULT_OK);
    assert(TFT_ILI9341_DrawMonoBitmap(&tft, 0, 90, 8U, 2U,
        custom_glyph, sizeof(custom_glyph), TFT_ILI9341_WHITE,
        TFT_ILI9341_BLACK, true) == SIGNAL_RESULT_OK);
    assert(TFT_ILI9341_DrawMonoBitmap(&tft, 12, 90, 16U, 16U,
        TFT_ILI9341_GLYPH_CN_DIAN_16X16,
        TFT_ILI9341_GLYPH_16X16_BYTES, TFT_ILI9341_WHITE,
        TFT_ILI9341_BLACK, false) == SIGNAL_RESULT_OK);
    assert(TFT_ILI9341_DrawMonoBitmap(&tft, 28, 90, 16U, 16U,
        TFT_ILI9341_GLYPH_CN_ZI_16X16,
        TFT_ILI9341_GLYPH_16X16_BYTES, TFT_ILI9341_WHITE,
        TFT_ILI9341_BLACK, false) == SIGNAL_RESULT_OK);
    assert(TFT_ILI9341_DrawMonoBitmap(&tft, 0, 90, 8U, 2U,
        custom_glyph, 1U, TFT_ILI9341_WHITE,
        TFT_ILI9341_BLACK, true) == SIGNAL_RESULT_INVALID_ARGUMENT);
    assert(SignalTFTILI9341_GetModuleStatus() ==
        MODULE_STATUS_BUILD_VERIFIED);
}

static void TestTFTWaveform(void)
{
    tft_mock_t mock = { false, false, false, false, 0U, 0U, 0U, 0U, 0U };
    tft_ili9341_t tft;
    int32_t y;
    float low;
    float high;
    float pulse[16] = { 0.0F };
    float flat[4] = { 1.0F, 1.0F, 1.0F, 1.0F };
    signal_tft_waveform_result_t plotted;
    signal_tft_waveform_config_t view = {
        0, 0, 8U, 16U,
        SIGNAL_TFT_WAVEFORM_MIN_MAX_ENVELOPE,
        SIGNAL_TFT_WAVEFORM_FIXED_SCALE,
        0.0F, 1.0F, 0.5F,
        TFT_ILI9341_YELLOW, TFT_ILI9341_BLACK,
        TFT_ILI9341_BLUE, TFT_ILI9341_CYAN,
        4U, 4U, true, true, true, true
    };
    const tft_ili9341_config_t tft_config = {
        &mock,
        MockTFTWrite,
        MockTFTSetCS,
        MockTFTSetDC,
        MockTFTSetReset,
        MockTFTSetBacklight,
        MockTFTDelay,
        NULL,
        NULL
    };

    assert(SignalTFTWaveform_MapY(1.0F, 0.0F, 1.0F,
        10, 11U, &y) == SIGNAL_RESULT_OK);
    assert(y == 10);
    assert(SignalTFTWaveform_MapY(-1.0F, 0.0F, 1.0F,
        10, 11U, &y) == SIGNAL_RESULT_OK);
    assert(y == 20);
    assert(SignalTFTWaveform_MapY(0.5F, 1.0F, 1.0F,
        0, 10U, &y) == SIGNAL_RESULT_INVALID_ARGUMENT);

    pulse[5] = 1.0F;
    assert(SignalTFTWaveform_GetEnvelopeColumn(pulse, 16U, 4U, 1U,
        &low, &high) == SIGNAL_RESULT_OK);
    assert((low == 0.0F) && (high == 1.0F));
    assert(SignalTFTWaveform_GetEnvelopeColumn(pulse, 16U, 17U, 0U,
        &low, &high) == SIGNAL_RESULT_INVALID_ARGUMENT);

    assert(TFT_ILI9341_Init(&tft, &tft_config, TFT_ILI9341_ROTATION_90) ==
        SIGNAL_RESULT_OK);
    assert(SignalTFTWaveform_Draw(&tft, pulse, 16U, &view, &plotted) ==
        SIGNAL_RESULT_OK);
    assert((plotted.plotted_columns == 8U) &&
           (plotted.data_minimum == 0.0F) &&
           (plotted.data_maximum == 1.0F));

    view.mode = SIGNAL_TFT_WAVEFORM_DECIMATE;
    view.scale_mode = SIGNAL_TFT_WAVEFORM_AUTO_SCALE;
    assert(SignalTFTWaveform_Draw(&tft, flat, 4U, &view, &plotted) ==
        SIGNAL_RESULT_OK);
    assert((plotted.plotted_columns == 4U) &&
           (plotted.scale_minimum < 1.0F) &&
           (plotted.scale_maximum > 1.0F));
    assert(SignalTFTWaveform_GetModuleStatus() ==
        MODULE_STATUS_BUILD_VERIFIED);
}

static void TestRotaryEncoder(void)
{
    rotary_encoder_mock_t mock = { false, false, true };
    signal_rotary_encoder_t encoder;
    signal_rotary_encoder_event_t event;
    int32_t position;
    uint32_t invalid_count;
    const signal_rotary_encoder_config_t config = {
        &mock, MockRotaryA, MockRotaryB, MockRotaryButton,
        4U, 3U, true
    };

    assert(SignalRotaryEncoder_Init(&encoder, &config) == SIGNAL_RESULT_OK);

    /* 00 -> 01 -> 11 -> 10 -> 00 is one decoded direction. */
    mock.b_high = true;
    assert(SignalRotaryEncoder_Update(&encoder, &event) == SIGNAL_RESULT_OK);
    assert(event.step_delta == 0);
    mock.a_high = true;
    assert(SignalRotaryEncoder_Update(&encoder, &event) == SIGNAL_RESULT_OK);
    mock.b_high = false;
    assert(SignalRotaryEncoder_Update(&encoder, &event) == SIGNAL_RESULT_OK);
    mock.a_high = false;
    assert(SignalRotaryEncoder_Update(&encoder, &event) == SIGNAL_RESULT_OK);
    assert((event.step_delta == -1) && (event.position == -1));

    /* Reverse sequence returns to zero. */
    mock.a_high = true;
    assert(SignalRotaryEncoder_Update(&encoder, &event) == SIGNAL_RESULT_OK);
    mock.b_high = true;
    assert(SignalRotaryEncoder_Update(&encoder, &event) == SIGNAL_RESULT_OK);
    mock.a_high = false;
    assert(SignalRotaryEncoder_Update(&encoder, &event) == SIGNAL_RESULT_OK);
    mock.b_high = false;
    assert(SignalRotaryEncoder_Update(&encoder, &event) == SIGNAL_RESULT_OK);
    assert((event.step_delta == 1) && (event.position == 0));

    /* Opposite-state jump means polling missed an intermediate state. */
    mock.a_high = true;
    mock.b_high = true;
    assert(SignalRotaryEncoder_Update(&encoder, &event) == SIGNAL_RESULT_OK);
    assert(event.invalid_transition);
    assert(SignalRotaryEncoder_GetInvalidTransitionCount(
        &encoder, &invalid_count) == SIGNAL_RESULT_OK);
    assert(invalid_count == 1U);

    /* Active-low button must stay low for three updates. */
    mock.button_high = false;
    assert(SignalRotaryEncoder_Update(&encoder, &event) == SIGNAL_RESULT_OK);
    assert(!event.button_pressed);
    assert(SignalRotaryEncoder_Update(&encoder, &event) == SIGNAL_RESULT_OK);
    assert(!event.button_pressed);
    assert(SignalRotaryEncoder_Update(&encoder, &event) == SIGNAL_RESULT_OK);
    assert(event.button_pressed && event.stable_button_pressed);

    assert(SignalRotaryEncoder_SetPosition(&encoder, INT32_MAX) ==
        SIGNAL_RESULT_OK);
    assert(SignalRotaryEncoder_GetPosition(&encoder, &position) ==
        SIGNAL_RESULT_OK);
    assert(position == INT32_MAX);
    assert(SignalRotaryEncoder_GetModuleStatus() ==
        MODULE_STATUS_BUILD_VERIFIED);
}

static void TestButton(void)
{
    digital_input_mock_t mock = { false, 0U };
    signal_button_t button;
    signal_button_event_t event;
    const signal_button_config_t config = {
        &mock,
        MockDigitalInput,
        3U
    };
    bool pressed;

    assert(SignalButton_Init(&button, &config) == SIGNAL_RESULT_OK);
    mock.value = true;
    assert(SignalButton_Update(&button, &event) == SIGNAL_RESULT_OK);
    assert(!event.pressed);
    assert(SignalButton_Update(&button, &event) == SIGNAL_RESULT_OK);
    assert(!event.pressed);
    assert(SignalButton_Update(&button, &event) == SIGNAL_RESULT_OK);
    assert(event.pressed && event.stable_pressed);
    assert(SignalButton_GetPressed(&button, &pressed) == SIGNAL_RESULT_OK);
    assert(pressed);

    mock.value = false;
    assert(SignalButton_Update(&button, &event) == SIGNAL_RESULT_OK);
    assert(SignalButton_Update(&button, &event) == SIGNAL_RESULT_OK);
    assert(SignalButton_Update(&button, &event) == SIGNAL_RESULT_OK);
    assert(event.released && !event.stable_pressed);
    assert(mock.reads == 6U);
    assert(SignalButton_GetModuleStatus() == MODULE_STATUS_BUILD_VERIFIED);
}

static void TestLatchingButtonSwitch(void)
{
    digital_input_mock_t mock = { true, 0U };
    signal_latching_button_switch_t switch_module;
    signal_latching_button_switch_event_t event;
    const signal_latching_button_switch_config_t config = {
        &mock,
        MockDigitalInput,
        3U
    };
    bool on;

    assert(SignalLatchingButtonSwitch_Init(&switch_module, &config) ==
        SIGNAL_RESULT_OK);
    assert(SignalLatchingButtonSwitch_GetState(&switch_module, &on) ==
        SIGNAL_RESULT_NO_DATA);
    assert(SignalLatchingButtonSwitch_Update(&switch_module, &event) ==
        SIGNAL_RESULT_OK);
    assert(!event.state_valid);
    assert(SignalLatchingButtonSwitch_Update(&switch_module, &event) ==
        SIGNAL_RESULT_OK);
    assert(SignalLatchingButtonSwitch_Update(&switch_module, &event) ==
        SIGNAL_RESULT_OK);
    assert(event.state_valid && event.stable_on && !event.changed);
    assert(SignalLatchingButtonSwitch_GetState(&switch_module, &on) ==
        SIGNAL_RESULT_OK);
    assert(on);

    mock.value = false;
    assert(SignalLatchingButtonSwitch_Update(&switch_module, &event) ==
        SIGNAL_RESULT_OK);
    assert(SignalLatchingButtonSwitch_Update(&switch_module, &event) ==
        SIGNAL_RESULT_OK);
    assert(SignalLatchingButtonSwitch_Update(&switch_module, &event) ==
        SIGNAL_RESULT_OK);
    assert(event.changed && event.turned_off && !event.turned_on);

    mock.value = true;
    assert(SignalLatchingButtonSwitch_Update(&switch_module, &event) ==
        SIGNAL_RESULT_OK);
    assert(SignalLatchingButtonSwitch_Update(&switch_module, &event) ==
        SIGNAL_RESULT_OK);
    assert(SignalLatchingButtonSwitch_Update(&switch_module, &event) ==
        SIGNAL_RESULT_OK);
    assert(event.changed && event.turned_on && !event.turned_off);
    assert(SignalLatchingButtonSwitch_GetModuleStatus() ==
        MODULE_STATUS_BUILD_VERIFIED);
}

int main(void)
{
    TestMeasurements();
    TestTimeAndFrequency();
    TestSpectrum();
    TestSineFit();
    TestTHDAndWindowCorrection();
    TestDDSAndBuffers();
    TestCaptureMath();
    TestMatrixKeypad4x4();
    TestTFTILI9341();
    TestTFTWaveform();
    TestRotaryEncoder();
    TestButton();
    TestLatchingButtonSwitch();
    puts("signal_library_tests: PASS");
    return 0;
}
