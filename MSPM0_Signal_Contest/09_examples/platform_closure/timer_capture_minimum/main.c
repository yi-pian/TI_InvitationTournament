#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_comparator.h"
#include "signal_mspm0g3507_capture_platform.h"
#include "signal_mspm0g3507_platform.h"
#include "signal_timer_capture.h"

#define CAPTURE_COUNT 8U

static volatile uint32_t g_timestamp_isr[CAPTURE_COUNT];
static uint32_t g_timestamps[CAPTURE_COUNT];
static signal_mspm0g3507_capture_t g_capture;
volatile float g_frequency_hz;
volatile int32_t g_capture_status;

int main(void)
{
    signal_comparator_t comparator;
    signal_mspm0g3507_comparator_context_t comparator_context = {
        .instance = SIGNAL_COMP_INST,
        .reference_voltage_v = 3.3f,
    };
    const signal_comparator_config_t comparator_config = {
        .threshold_v = 1.65f,
        .hysteresis_v = 0.030f,
        .invert_output = false,
    };
    const signal_timer_capture_config_t capture_config = {
        .timer_hz = CPUCLK_FREQ,
        .counter_modulus = SIGNAL_CAPTURE_INST_LOAD_VALUE + 1U,
    };
    size_t timestamp_count = 0U;
    float mean_ticks;
    float frequency_hz;

    SYSCFG_DL_init();
    g_capture_status = (int32_t) SignalMSPM0G3507_Comparator_Bind(
        &comparator, &comparator_context);
    if (g_capture_status == (int32_t) SIGNAL_RESULT_OK) {
        g_capture_status = (int32_t) SignalComparator_Apply(
            &comparator, &comparator_config, 3.3f);
    }
    if (g_capture_status == (int32_t) SIGNAL_RESULT_OK) {
        g_capture_status = (int32_t) SignalMSPM0G3507_Capture_Init(
            &g_capture, g_timestamp_isr, CAPTURE_COUNT,
            SIGNAL_CAPTURE_INST_LOAD_VALUE + 1U, 100U);
    }
    if (g_capture_status == (int32_t) SIGNAL_RESULT_OK) {
        g_capture_status = (int32_t) SignalMSPM0G3507_Capture_Start(
            &g_capture);
    }
    while ((g_capture_status == (int32_t) SIGNAL_RESULT_OK) &&
           !SignalMSPM0G3507_Capture_IsFinished(&g_capture)) {
        __WFE();
    }
    (void) SignalMSPM0G3507_Capture_Stop(&g_capture);
    if (g_capture_status == (int32_t) SIGNAL_RESULT_OK) {
        g_capture_status = (int32_t) SignalMSPM0G3507_Capture_Copy(
            &g_capture, g_timestamps, CAPTURE_COUNT, &timestamp_count);
    }
    if ((g_capture_status == (int32_t) SIGNAL_RESULT_OK) &&
        (timestamp_count >= 2U)) {
        g_capture_status = (int32_t) SignalTimerCapture_MeanPeriod(
            g_timestamps, timestamp_count, &capture_config,
            &mean_ticks, &frequency_hz);
        g_frequency_hz = frequency_hz;
    }
    while (1) __WFI();
}
