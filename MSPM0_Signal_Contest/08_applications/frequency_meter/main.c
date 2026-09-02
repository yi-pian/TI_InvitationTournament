#include <stdbool.h>
#include <stdint.h>

#include "signal_config.h"
#if SIGNAL_FREQUENCY_METHOD != SIGNAL_FREQUENCY_METHOD_CAPTURE
#include "signal_integration.h"
#endif
#include "ti_msp_dl_config.h"

volatile float g_frequency_hz;
volatile uint32_t g_frequency_measurement_ready;
volatile int32_t g_frequency_status;

static void FrequencyMeter_Fail(int32_t status)
{
    g_frequency_status = status;
    __BKPT(0);
    while (1) {
        __WFI();
    }
}

#if SIGNAL_FREQUENCY_METHOD == SIGNAL_FREQUENCY_METHOD_CAPTURE

#include "signal_mspm0g3507_capture_platform.h"
#include "signal_timer_capture.h"

static volatile uint32_t g_capture_timestamps[SIGNAL_CAPTURE_TIMESTAMP_COUNT];
static uint32_t g_capture_snapshot[SIGNAL_CAPTURE_TIMESTAMP_COUNT];
static signal_mspm0g3507_capture_t g_capture;

int main(void)
{
    signal_timer_capture_config_t capture_config = {
        .timer_hz = SIGNAL_CAPTURE_CLOCK_HZ,
        .counter_modulus = SIGNAL_CAPTURE_COUNTER_MODULUS,
    };
    float mean_ticks;
    float frequency_hz;
    size_t capture_count;

    SYSCFG_DL_init();
    if (SignalMSPM0G3507_Capture_Init(&g_capture, g_capture_timestamps,
            SIGNAL_CAPTURE_TIMESTAMP_COUNT,
            SIGNAL_CAPTURE_COUNTER_MODULUS,
            SIGNAL_CAPTURE_TIMEOUT_OVERFLOWS) != SIGNAL_RESULT_OK) {
        FrequencyMeter_Fail((int32_t) SIGNAL_RESULT_INVALID_ARGUMENT);
    }
    if (SignalMSPM0G3507_Capture_Start(&g_capture) != SIGNAL_RESULT_OK) {
        FrequencyMeter_Fail((int32_t) SIGNAL_RESULT_HARDWARE_ERROR);
    }
    while (!SignalMSPM0G3507_Capture_IsFinished(&g_capture)) {
        __WFE();
    }
    (void) SignalMSPM0G3507_Capture_Stop(&g_capture);
    if (SignalMSPM0G3507_Capture_Copy(&g_capture, g_capture_snapshot,
            SIGNAL_CAPTURE_TIMESTAMP_COUNT, &capture_count) !=
        SIGNAL_RESULT_OK) {
        FrequencyMeter_Fail((int32_t) SIGNAL_RESULT_NO_DATA);
    }
    if (capture_count < 2U) {
        FrequencyMeter_Fail((int32_t) SIGNAL_RESULT_NO_DATA);
    }
    g_frequency_status = (int32_t) SignalTimerCapture_MeanPeriod(
        g_capture_snapshot, capture_count, &capture_config,
        &mean_ticks, &frequency_hz);
    if (g_frequency_status != (int32_t) SIGNAL_RESULT_OK) {
        FrequencyMeter_Fail(g_frequency_status);
    }
    g_frequency_hz = frequency_hz;
    g_frequency_measurement_ready = 1U;
    __BKPT(0);
    while (1) __WFI();
}

#else

#include "signal_adc_dma.h"

#define SIGNAL_EVENT_CAPACITY ((SIGNAL_SAMPLE_COUNT / 2U) + 1U)
static uint16_t g_raw[SIGNAL_SAMPLE_COUNT];
static float g_voltage[SIGNAL_SAMPLE_COUNT];

#if SIGNAL_FREQUENCY_METHOD == SIGNAL_FREQUENCY_METHOD_ZERO_CROSS
static signal_zero_cross_event_t g_events[SIGNAL_EVENT_CAPACITY];
static float g_positions[SIGNAL_EVENT_CAPACITY];
#else
static signal_complex_f32_t g_fft[SIGNAL_SAMPLE_COUNT];
static float g_magnitude[(SIGNAL_SAMPLE_COUNT / 2U) + 1U];
#endif

int main(void)
{
    signal_adc_dma_config_t adc_config = {
        .sample_rate_hz = SIGNAL_SAMPLE_RATE_HZ,
        .timer_clock_hz = CPUCLK_FREQ,
        .timer_max_count = 65536U,
    };
    signal_algorithm_status_t status;

    SYSCFG_DL_init();
    if (SignalADC_Init(&adc_config) != SIGNAL_RESULT_OK) {
        FrequencyMeter_Fail(-100);
    }
    if (SignalADC_Start(g_raw, SIGNAL_SAMPLE_COUNT) != SIGNAL_RESULT_OK) {
        FrequencyMeter_Fail(-101);
    }
    while (!SignalADC_IsFinished()) __WFE();
    status = SignalIntegration_RawToVoltage(SignalADC_GetBuffer(),
        SignalADC_GetSampleCount(), SIGNAL_ADC_BITS, SIGNAL_ADC_VREF_V,
        SIGNAL_INPUT_SCALE, SIGNAL_INPUT_OFFSET_V, g_voltage,
        SIGNAL_SAMPLE_COUNT);
    if (status != SIGNAL_ALGORITHM_OK) FrequencyMeter_Fail((int32_t) status);

#if SIGNAL_FREQUENCY_METHOD == SIGNAL_FREQUENCY_METHOD_ZERO_CROSS
    {
        float measured_frequency_hz;
        status = SignalIntegration_FrequencyTime(g_voltage,
            SIGNAL_SAMPLE_COUNT,
            (float) SignalADC_GetConfiguredTriggerRate(),
            SIGNAL_ZERO_CROSS_HYSTERESIS_V, g_events,
            SIGNAL_EVENT_CAPACITY, g_positions, SIGNAL_EVENT_CAPACITY,
            &measured_frequency_hz);
        if (status == SIGNAL_ALGORITHM_OK) {
            g_frequency_hz = measured_frequency_hz;
        }
    }
#else
    {
        signal_spectrum_integration_result_t spectrum;
        status = SignalIntegration_Spectrum(g_voltage, SIGNAL_SAMPLE_COUNT,
            (float) SignalADC_GetConfiguredTriggerRate(),
            (float) SIGNAL_EXPECTED_FREQ_MIN_HZ,
            (float) SIGNAL_EXPECTED_FREQ_MAX_HZ, g_fft,
            SIGNAL_SAMPLE_COUNT, g_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U, 1U, &spectrum);
        if (status == SIGNAL_ALGORITHM_OK) {
            g_frequency_hz = spectrum.frequency_hz;
        }
    }
#endif
    if (status != SIGNAL_ALGORITHM_OK) FrequencyMeter_Fail((int32_t) status);
    g_frequency_status = (int32_t) status;
    g_frequency_measurement_ready = 1U;
    __BKPT(0);
    while (1) __WFI();
}

#endif
