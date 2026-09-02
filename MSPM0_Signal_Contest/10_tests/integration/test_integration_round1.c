#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "signal_integration.h"

#define PI_F (3.14159265358979323846f)
#define METER_N (1024U)
#define PHASE_N (512U)

static uint16_t s_raw[METER_N];
static float s_voltage[METER_N];
static signal_zero_cross_event_t s_events[METER_N / 2U + 1U];
static float s_positions[METER_N / 2U + 1U];
static signal_complex_f32_t s_fft[METER_N];
static float s_magnitude[METER_N / 2U + 1U];

static float s_phase_a[PHASE_N];
static float s_phase_b[PHASE_N];
static signal_complex_f32_t s_fft_a[PHASE_N];
static signal_complex_f32_t s_fft_b[PHASE_N];
static float s_correlation[129U];

static int Near(float actual, float expected, float tolerance)
{
    return fabsf(actual - expected) <= tolerance;
}

static uint16_t VoltageToRaw(float voltage_v)
{
    float code = voltage_v * 4095.0f / 3.3f;
    if (code < 0.0f) code = 0.0f;
    if (code > 4095.0f) code = 4095.0f;
    return (uint16_t) (code + 0.5f);
}

static void FillRaw(float h2, float h3)
{
    uint32_t i;
    const float fs = 102400.0f;
    const float f = 1000.0f;
    for (i = 0U; i < METER_N; ++i) {
        float phase = 2.0f * PI_F * f * (float) i / fs;
        float voltage = 1.65f + 0.5f * sinf(phase) +
            h2 * sinf(2.0f * phase + 0.2f) +
            h3 * sinf(3.0f * phase - 0.1f);
        s_raw[i] = VoltageToRaw(voltage);
    }
}

static void TestSignalMeter(void)
{
    signal_meter_result_t result;
    FillRaw(0.0f, 0.0f);
    assert(SignalIntegration_SignalMeter(s_raw, METER_N, 12U, 3.3f,
        1.0f, 0.0f, 102400.0f,
        SIGNAL_METER_MEASURE_DC | SIGNAL_METER_MEASURE_MIN_MAX |
        SIGNAL_METER_MEASURE_VPP | SIGNAL_METER_MEASURE_RMS |
        SIGNAL_METER_MEASURE_AC_RMS | SIGNAL_METER_MEASURE_FREQUENCY,
        0.001f, s_voltage, METER_N, s_events, METER_N / 2U + 1U,
        s_positions, METER_N / 2U + 1U, &result) == SIGNAL_ALGORITHM_OK);
    assert(Near(result.dc_v, 1.65f, 0.002f));
    assert(Near(result.vpp_v, 1.0f, 0.003f));
    assert(Near(result.ac_rms_v, 0.353553f, 0.002f));
    assert(Near(result.frequency_hz, 1000.0f, 0.5f));
    puts("round1 signal_meter: PASS");
}

static void TestSpectrum(void)
{
    signal_spectrum_integration_result_t result;
    FillRaw(0.0f, 0.0f);
    assert(SignalIntegration_RawToVoltage(s_raw, METER_N, 12U, 3.3f,
        1.0f, 0.0f, s_voltage, METER_N) == SIGNAL_ALGORITHM_OK);
    assert(SignalIntegration_Spectrum(s_voltage, METER_N, 102400.0f,
        100.0f, 20000.0f, s_fft, METER_N, s_magnitude,
        METER_N / 2U + 1U, 3U, &result) == SIGNAL_ALGORITHM_OK);
    assert(Near(result.frequency_hz, 1000.0f, 0.5f));
    assert(Near(result.main_peak_v, 0.5f, 0.01f));
    puts("round1 spectrum: PASS");
}

static void TestTHD(void)
{
    signal_thd_integration_result_t result;
    FillRaw(0.05f, 0.025f);
    assert(SignalIntegration_RawToVoltage(s_raw, METER_N, 12U, 3.3f,
        1.0f, 0.0f, s_voltage, METER_N) == SIGNAL_ALGORITHM_OK);
    assert(SignalIntegration_THD(s_voltage, METER_N, 102400.0f,
        100.0f, 9000.0f, 2U, s_fft, METER_N, s_magnitude,
        METER_N / 2U + 1U, &result) == SIGNAL_ALGORITHM_OK);
    assert(Near(result.fundamental_frequency_hz, 1000.0f, 0.5f));
    assert(Near(result.thd_percent, 11.18034f, 0.7f));
    puts("round1 thd: PASS");
}

static void TestPhase(void)
{
    signal_phase_integration_result_t result;
    uint32_t i;
    for (i = 0U; i < PHASE_N; ++i) {
        float p = 2.0f * PI_F * 1000.0f * (float) i / 51200.0f;
        s_phase_a[i] = 1.0f + sinf(p);
        s_phase_b[i] = 1.0f + sinf(p - PI_F / 4.0f);
    }
    assert(SignalIntegration_DualPhase(s_phase_a, s_phase_b, PHASE_N,
        51200.0f, 1000.0f, 64U, s_fft_a, s_fft_b, PHASE_N,
        s_correlation, 129U, &result) == SIGNAL_ALGORITHM_OK);
    assert(Near(result.fft_phase_deg, -45.0f, 1.0f));
    assert(Near(result.correlation_phase_deg, -45.0f, 4.0f));
    puts("round1 phase: PASS");
}

int main(void)
{
    TestSignalMeter();
    TestSpectrum();
    TestTHD();
    TestPhase();
    puts("integration round1: PASS");
    return 0;
}
