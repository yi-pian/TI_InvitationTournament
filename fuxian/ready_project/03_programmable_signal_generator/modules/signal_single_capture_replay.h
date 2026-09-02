#ifndef SIGNAL_SINGLE_CAPTURE_REPLAY_H
#define SIGNAL_SINGLE_CAPTURE_REPLAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "signal_status.h"
#include "signal_tft_st7789.h"
#include "signal_trigger_capture.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*signal_single_capture_clear_trigger_fn)(void *context);
typedef bool (*signal_single_capture_consume_trigger_fn)(void *context);

typedef struct {
    uint16_t *adc_channel_a;
    uint16_t *adc_channel_b;
    size_t adc_capacity_per_channel;
    uint16_t *search_buffer;
    size_t search_capacity;
    uint16_t *slot_samples;
    size_t slot_sample_capacity;
    uint16_t *slot_lengths;
    uint32_t *slot_sample_rates_hz;
    bool *slot_valid;
    uint16_t *replay_table;
    size_t replay_table_capacity;
    uint16_t samples_per_block;
    uint8_t dma_block_count;
    uint8_t slot_count;
    uint16_t pretrigger_samples;
    uint16_t baseline_samples;
    uint16_t edge_margin_samples;
    uint16_t minimum_activity_codes;
    uint16_t quiet_tail_samples;
    uint16_t activity_run_samples;
    uint16_t adc_max_code;
    uint16_t trigger_level_code;
    uint16_t trigger_hysteresis_code;
    signal_trigger_edge_t trigger_edge;
    uint32_t requested_sample_rate_hz;
    signal_single_capture_clear_trigger_fn clear_trigger;
    signal_single_capture_consume_trigger_fn consume_trigger;
    void *trigger_context;
} signal_single_capture_replay_config_t;

typedef struct {
    signal_single_capture_replay_config_t config;
    uint32_t effective_sample_rate_hz;
    uint32_t dma_sequence;
    uint16_t pending_trigger_index;
    uint8_t selected_slot;
    uint8_t next_slot;
    bool initialized;
    bool armed;
    bool previous_block_valid;
    bool trigger_pending;
    volatile bool trigger_seen;
} signal_single_capture_replay_t;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    uint16_t waveform_color;
    uint16_t background_color;
    bool clear_background;
} signal_single_capture_plot_config_t;

typedef struct {
    uint16_t sample_count;
    uint32_t sample_rate_hz;
    uint32_t duration_us;
    uint16_t minimum_code;
    uint16_t maximum_code;
} signal_single_capture_info_t;

signal_result_t SignalSingleCaptureReplay_Init(
    signal_single_capture_replay_t *capture,
    const signal_single_capture_replay_config_t *config);
signal_result_t SignalSingleCaptureReplay_Arm(
    signal_single_capture_replay_t *capture);
void SignalSingleCaptureReplay_Cancel(
    signal_single_capture_replay_t *capture);
void SignalSingleCaptureReplay_NotifyTrigger(
    signal_single_capture_replay_t *capture);
signal_result_t SignalSingleCaptureReplay_Service(
    signal_single_capture_replay_t *capture);
bool SignalSingleCaptureReplay_IsArmed(
    const signal_single_capture_replay_t *capture);
signal_result_t SignalSingleCaptureReplay_SelectSlot(
    signal_single_capture_replay_t *capture, uint8_t slot_index);
uint8_t SignalSingleCaptureReplay_GetSelectedSlot(
    const signal_single_capture_replay_t *capture);
uint8_t SignalSingleCaptureReplay_GetNextSlot(
    const signal_single_capture_replay_t *capture);
signal_result_t SignalSingleCaptureReplay_GetSelected(
    const signal_single_capture_replay_t *capture,
    const uint16_t **samples, signal_single_capture_info_t *info);
signal_result_t SignalSingleCaptureReplay_ReplaySelected(
    signal_single_capture_replay_t *capture);
signal_result_t SignalSingleCaptureReplay_DrawSelectedST7789(
    const signal_single_capture_replay_t *capture, tft_st7789_t *tft,
    const signal_single_capture_plot_config_t *plot,
    signal_single_capture_info_t *info);
signal_module_status_t SignalSingleCaptureReplay_GetModuleStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_SINGLE_CAPTURE_REPLAY_H */
