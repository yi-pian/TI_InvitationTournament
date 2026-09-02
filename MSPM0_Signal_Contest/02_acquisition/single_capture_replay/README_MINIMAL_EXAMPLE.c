/* 最小闭环：D 键调用 Arm，主循环调用 Service，捕获成功后显示并回放。 */
#include <stdbool.h>
#include <stdint.h>

#include "signal_single_capture_replay.h"

#define CAPTURE_SAMPLES (416U)
#define CAPTURE_BLOCKS  (3U)
#define CAPTURE_SLOTS   (3U)
#define REPLAY_SAMPLES  (256U)

static uint16_t g_adc_a[CAPTURE_SAMPLES * CAPTURE_BLOCKS];
static uint16_t g_adc_b[CAPTURE_SAMPLES * CAPTURE_BLOCKS];
static uint16_t g_search[CAPTURE_SAMPLES * 2U];
static uint16_t g_slots[CAPTURE_SLOTS][CAPTURE_SAMPLES];
static uint16_t g_lengths[CAPTURE_SLOTS];
static uint32_t g_rates[CAPTURE_SLOTS];
static bool g_valid[CAPTURE_SLOTS];
static uint16_t g_replay[REPLAY_SAMPLES];
static signal_single_capture_replay_t g_capture;

/* 这两个回调内使用本工程 SysConfig 生成的 COMP 实例宏。 */
static void ClearComparatorTrigger(void *context) { (void)context; }
static bool ConsumeComparatorTrigger(void *context)
{
    (void)context;
    return false;
}

void SignalSingleCaptureReplay_MinimalInit(void)
{
    const signal_single_capture_replay_config_t config = {
        .adc_channel_a = g_adc_a,
        .adc_channel_b = g_adc_b,
        .adc_capacity_per_channel = CAPTURE_SAMPLES * CAPTURE_BLOCKS,
        .search_buffer = g_search,
        .search_capacity = CAPTURE_SAMPLES * 2U,
        .slot_samples = &g_slots[0][0],
        .slot_sample_capacity = CAPTURE_SLOTS * CAPTURE_SAMPLES,
        .slot_lengths = g_lengths,
        .slot_sample_rates_hz = g_rates,
        .slot_valid = g_valid,
        .replay_table = g_replay,
        .replay_table_capacity = REPLAY_SAMPLES,
        .samples_per_block = CAPTURE_SAMPLES,
        .dma_block_count = CAPTURE_BLOCKS,
        .slot_count = CAPTURE_SLOTS,
        .pretrigger_samples = 208U,
        .baseline_samples = 32U,
        .edge_margin_samples = 4U,
        .minimum_activity_codes = 32U,
        .quiet_tail_samples = 8U,
        .activity_run_samples = 3U,
        .adc_max_code = 4095U,
        .trigger_level_code = 2048U,
        .trigger_hysteresis_code = 32U,
        .trigger_edge = SIGNAL_TRIGGER_EITHER,
        .requested_sample_rate_hz = 1000000U,
        .clear_trigger = ClearComparatorTrigger,
        .consume_trigger = ConsumeComparatorTrigger,
        .trigger_context = NULL
    };
    (void)SignalSingleCaptureReplay_Init(&g_capture, &config);
}

/* COMP ISR 清除硬件中断后只调用这一句。 */
void SignalSingleCaptureReplay_MinimalComparatorISR(void)
{
    SignalSingleCaptureReplay_NotifyTrigger(&g_capture);
}

void SignalSingleCaptureReplay_MinimalLoop(void)
{
    if (SignalSingleCaptureReplay_Service(&g_capture) == SIGNAL_RESULT_OK) {
        (void)SignalSingleCaptureReplay_ReplaySelected(&g_capture);
    }
}
