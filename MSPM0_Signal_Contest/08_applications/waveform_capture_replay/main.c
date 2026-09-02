#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "signal_adc_dma.h"
#include "signal_adc_ring_buffer.h"
#include "signal_config.h"
#include "signal_dac_dma.h"
#include "signal_dac_dma_platform.h"
#include "signal_trigger_capture.h"
#include "signal_waveform_capture_replay.h"
#include "ti_msp_dl_config.h"

static uint16_t g_dma_capture[SIGNAL_CAPTURE_SAMPLE_COUNT];
/* Ring module reserves one slot to distinguish full from empty. */
static uint16_t g_ring_storage[SIGNAL_CAPTURE_SAMPLE_COUNT + 1U];
static uint16_t g_ordered_capture[SIGNAL_CAPTURE_SAMPLE_COUNT];
static uint16_t g_period_segment[SIGNAL_CAPTURE_SAMPLE_COUNT];
static uint16_t g_replay_table[SIGNAL_REPLAY_TABLE_COUNT];

volatile size_t g_replay_trigger_index;
volatile size_t g_replay_period_samples;
volatile uint16_t g_replay_captured_min;
volatile uint16_t g_replay_captured_max;
volatile uint32_t g_replay_update_rate_hz;
volatile int32_t g_replay_status;

static signal_adc_ring_buffer_t g_ring;
static signal_dac_dma_t g_dac_dma;

static void Replay_Fail(int32_t status)
{
    g_replay_status = status;
    __BKPT(0);
    while (1) { __WFI(); }
}

int main(void)
{
    signal_adc_dma_config_t adc_config = {
        SIGNAL_CAPTURE_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
    };
    signal_trigger_config_t trigger_config = {
        SIGNAL_TRIGGER_LEVEL_CODE, SIGNAL_TRIGGER_HYSTERESIS_CODE,
        SIGNAL_TRIGGER_EDGE
    };
    size_t first_trigger;
    size_t second_trigger;
    size_t index;
    uint16_t minimum;
    uint16_t maximum;
    uint32_t capture_rate;
    uint32_t replay_rate;
    signal_result_t status;

    SYSCFG_DL_init();
    status = SignalADCRing_Init(&g_ring, g_ring_storage,
        SIGNAL_CAPTURE_SAMPLE_COUNT + 1U);
    if (status != SIGNAL_RESULT_OK) { Replay_Fail(status); }
    status = SignalADC_Init(&adc_config);
    if (status != SIGNAL_RESULT_OK) { Replay_Fail(status); }
    status = SignalADC_Start(g_dma_capture, SIGNAL_CAPTURE_SAMPLE_COUNT);
    if (status != SIGNAL_RESULT_OK) { Replay_Fail(status); }
    while (!SignalADC_IsFinished()) { __WFI(); }
    capture_rate = SignalADC_GetConfiguredTriggerRate();

    for (index = 0U; index < SIGNAL_CAPTURE_SAMPLE_COUNT; ++index) {
        status = SignalADCRing_Push(&g_ring, g_dma_capture[index]);
        if (status != SIGNAL_RESULT_OK) { Replay_Fail(status); }
    }
    for (index = 0U; index < SIGNAL_CAPTURE_SAMPLE_COUNT; ++index) {
        status = SignalADCRing_Pop(&g_ring, &g_ordered_capture[index]);
        if (status != SIGNAL_RESULT_OK) { Replay_Fail(status); }
    }

    status = SignalTrigger_Find(g_ordered_capture,
        SIGNAL_CAPTURE_SAMPLE_COUNT, &trigger_config, 0U, &first_trigger);
    if (status != SIGNAL_RESULT_OK) { Replay_Fail(status); }
    if ((first_trigger + SIGNAL_TRIGGER_MIN_PERIOD_SAMPLES) >=
        (SIGNAL_CAPTURE_SAMPLE_COUNT - 1U)) {
        Replay_Fail(SIGNAL_RESULT_NO_DATA);
    }
    status = SignalTrigger_Find(g_ordered_capture,
        SIGNAL_CAPTURE_SAMPLE_COUNT, &trigger_config,
        first_trigger + SIGNAL_TRIGGER_MIN_PERIOD_SAMPLES - 1U,
        &second_trigger);
    if (status != SIGNAL_RESULT_OK) { Replay_Fail(status); }
    if (second_trigger <= first_trigger) { Replay_Fail(SIGNAL_RESULT_NO_DATA); }
    g_replay_period_samples = second_trigger - first_trigger;
    status = SignalTrigger_Extract(g_ordered_capture,
        SIGNAL_CAPTURE_SAMPLE_COUNT, first_trigger, 0U, g_period_segment,
        g_replay_period_samples);
    if (status != SIGNAL_RESULT_OK) { Replay_Fail(status); }
    status = SignalWaveformReplay_PrepareAutoRange(g_period_segment,
        g_replay_period_samples, SIGNAL_DAC_BITS, g_replay_table,
        SIGNAL_REPLAY_TABLE_COUNT, &minimum, &maximum);
    if (status != SIGNAL_RESULT_OK) { Replay_Fail(status); }

    replay_rate = (uint32_t) ((((uint64_t) capture_rate *
        SIGNAL_REPLAY_TABLE_COUNT) + g_replay_period_samples / 2U) /
        g_replay_period_samples);
    status = SignalDACPlatform_Init(replay_rate, CPUCLK_FREQ);
    if (status != SIGNAL_RESULT_OK) { Replay_Fail(status); }
    status = SignalDACDMA_Init(&g_dac_dma, NULL,
        SignalDACPlatform_Start, SignalDACPlatform_Stop);
    if (status != SIGNAL_RESULT_OK) { Replay_Fail(status); }
    status = SignalDACDMA_Start(&g_dac_dma, g_replay_table,
        SIGNAL_REPLAY_TABLE_COUNT, SIGNAL_REPLAY_REPEAT != 0U);
    if (status != SIGNAL_RESULT_OK) { Replay_Fail(status); }

    g_replay_trigger_index = first_trigger;
    g_replay_captured_min = minimum;
    g_replay_captured_max = maximum;
    g_replay_update_rate_hz = SignalDACPlatform_GetConfiguredRate();
    g_replay_status = SIGNAL_RESULT_OK;
    while (1) { __WFI(); }
}
