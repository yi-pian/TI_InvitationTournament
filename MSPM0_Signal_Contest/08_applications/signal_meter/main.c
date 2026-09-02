#include <stdint.h>

#include "signal_adc_dma.h"
#include "signal_config.h"
#include "signal_integration.h"
#include "ti_msp_dl_config.h"

#define SIGNAL_CROSSING_CAPACITY ((SIGNAL_SAMPLE_COUNT / 2U) + 1U)
#define SIGNAL_MEASUREMENT_MASK ( \
    (SIGNAL_ENABLE_DC ? SIGNAL_METER_MEASURE_DC : 0U) | \
    (SIGNAL_ENABLE_MIN_MAX ? SIGNAL_METER_MEASURE_MIN_MAX : 0U) | \
    (SIGNAL_ENABLE_VPP ? SIGNAL_METER_MEASURE_VPP : 0U) | \
    (SIGNAL_ENABLE_RMS ? SIGNAL_METER_MEASURE_RMS : 0U) | \
    (SIGNAL_ENABLE_AC_RMS ? SIGNAL_METER_MEASURE_AC_RMS : 0U) | \
    (SIGNAL_ENABLE_FREQUENCY ? SIGNAL_METER_MEASURE_FREQUENCY : 0U))

static uint16_t g_raw[SIGNAL_SAMPLE_COUNT];
static float g_voltage_v[SIGNAL_SAMPLE_COUNT];
static signal_zero_cross_event_t g_crossing_events[SIGNAL_CROSSING_CAPACITY];
static float g_crossing_positions[SIGNAL_CROSSING_CAPACITY];

signal_meter_result_t g_signal_meter_result;
volatile signal_result_t g_signal_meter_acquisition_status;
volatile signal_algorithm_status_t g_signal_meter_algorithm_status;
volatile uint32_t g_signal_meter_completed_frames;

static void SignalMeter_Fail(void)
{
    __BKPT(0);
    while (1) {
        __WFI();
    }
}

int main(void)
{
    const signal_adc_dma_config_t adc_config = {
        .sample_rate_hz = SIGNAL_SAMPLE_RATE_HZ,
        .timer_clock_hz = CPUCLK_FREQ,
        .timer_max_count = 65536U,
    };

    SYSCFG_DL_init();
    g_signal_meter_completed_frames = 0U;
    g_signal_meter_acquisition_status = SignalADC_Init(&adc_config);
    if (g_signal_meter_acquisition_status != SIGNAL_RESULT_OK) {
        SignalMeter_Fail();
    }

    do {
        g_signal_meter_acquisition_status =
            SignalADC_Start(g_raw, SIGNAL_SAMPLE_COUNT);
        if (g_signal_meter_acquisition_status != SIGNAL_RESULT_OK) {
            SignalMeter_Fail();
        }
        while (!SignalADC_IsFinished()) {
            __WFE();
        }

        g_signal_meter_algorithm_status = SignalIntegration_SignalMeter(
            SignalADC_GetBuffer(), SignalADC_GetSampleCount(),
            SIGNAL_ADC_BITS, SIGNAL_ADC_VREF_V, SIGNAL_INPUT_SCALE,
            SIGNAL_INPUT_OFFSET_V,
            (float) SignalADC_GetConfiguredTriggerRate(),
            SIGNAL_MEASUREMENT_MASK, SIGNAL_ZERO_CROSS_HYSTERESIS_V,
            g_voltage_v, SIGNAL_SAMPLE_COUNT, g_crossing_events,
            SIGNAL_CROSSING_CAPACITY, g_crossing_positions,
            SIGNAL_CROSSING_CAPACITY, &g_signal_meter_result);
        if (g_signal_meter_algorithm_status != SIGNAL_ALGORITHM_OK) {
            SignalMeter_Fail();
        }
        g_signal_meter_completed_frames++;
    } while (SIGNAL_RUN_CONTINUOUSLY != 0U);

    __BKPT(0);
    while (1) {
        __WFI();
    }
}
