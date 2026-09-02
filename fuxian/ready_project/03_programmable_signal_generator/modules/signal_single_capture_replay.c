#include "signal_single_capture_replay.h"

#include <string.h>

#include "signal_arbitrary_wave.h"
#include "signal_dac_dma_mspm0g3507.h"
#include "signal_dual_adc_mspm0g3507.h"

static bool configuration_valid(
    const signal_single_capture_replay_config_t *config)
{
    size_t adc_required;
    size_t slot_required;
    if ((config == NULL) || (config->adc_channel_a == NULL) ||
        (config->adc_channel_b == NULL) || (config->search_buffer == NULL) ||
        (config->slot_samples == NULL) || (config->slot_lengths == NULL) ||
        (config->slot_sample_rates_hz == NULL) ||
        (config->slot_valid == NULL) || (config->replay_table == NULL) ||
        (config->samples_per_block < 2U) || (config->dma_block_count < 2U) ||
        (config->slot_count == 0U) || (config->pretrigger_samples == 0U) ||
        (config->pretrigger_samples >= config->samples_per_block) ||
        (config->baseline_samples == 0U) ||
        (config->baseline_samples >= config->samples_per_block) ||
        (config->activity_run_samples == 0U) ||
        (config->adc_max_code == 0U) ||
        (config->requested_sample_rate_hz == 0U) ||
        (config->replay_table_capacity < 2U)) {
        return false;
    }
    adc_required = (size_t)config->samples_per_block *
        config->dma_block_count;
    slot_required = (size_t)config->samples_per_block * config->slot_count;
    return (config->adc_capacity_per_channel >= adc_required) &&
        (config->search_capacity >= (size_t)config->samples_per_block * 2U) &&
        (config->slot_sample_capacity >= slot_required);
}

static uint16_t *slot_pointer(signal_single_capture_replay_t *capture,
    uint8_t slot_index)
{
    return &capture->config.slot_samples[(size_t)slot_index *
        capture->config.samples_per_block];
}

static const uint16_t *const_slot_pointer(
    const signal_single_capture_replay_t *capture, uint8_t slot_index)
{
    return &capture->config.slot_samples[(size_t)slot_index *
        capture->config.samples_per_block];
}

static uint16_t trim_baseline(signal_single_capture_replay_t *capture,
    uint16_t *samples)
{
    const signal_single_capture_replay_config_t *config = &capture->config;
    uint32_t baseline_sum = 0U;
    uint32_t baseline;
    uint16_t minimum;
    uint16_t maximum;
    uint16_t threshold;
    uint16_t first_active = config->samples_per_block;
    uint16_t last_active = 0U;
    uint16_t active_run = 0U;
    uint16_t start;
    uint16_t end;
    uint16_t index;
    uint16_t result_count;

    for (index = 0U; index < config->baseline_samples; ++index) {
        baseline_sum += samples[index];
    }
    baseline = baseline_sum / config->baseline_samples;
    minimum = samples[0];
    maximum = samples[0];
    for (index = 1U; index < config->samples_per_block; ++index) {
        if (samples[index] < minimum) minimum = samples[index];
        if (samples[index] > maximum) maximum = samples[index];
    }
    threshold = (uint16_t)(((uint32_t)(maximum - minimum) + 11U) / 12U);
    if (threshold < config->minimum_activity_codes) {
        threshold = config->minimum_activity_codes;
    }
    for (index = 0U; index < config->samples_per_block; ++index) {
        uint32_t deviation = (samples[index] >= baseline) ?
            ((uint32_t)samples[index] - baseline) :
            (baseline - (uint32_t)samples[index]);
        if (deviation >= threshold) {
            ++active_run;
            if (active_run >= config->activity_run_samples) {
                uint16_t run_start = (uint16_t)(index + 1U - active_run);
                if (first_active == config->samples_per_block) {
                    first_active = run_start;
                }
                last_active = index;
            }
        } else {
            active_run = 0U;
        }
    }
    if ((first_active == config->samples_per_block) ||
        ((uint32_t)last_active + config->quiet_tail_samples >=
            config->samples_per_block)) {
        return 0U;
    }
    start = (first_active > config->edge_margin_samples) ?
        (uint16_t)(first_active - config->edge_margin_samples) : 0U;
    end = (uint16_t)(last_active + config->edge_margin_samples + 1U);
    if (end > config->samples_per_block) end = config->samples_per_block;
    result_count = (uint16_t)(end - start);
    memmove(samples, &samples[start], (size_t)result_count * sizeof(*samples));
    return result_count;
}

static void roll_search_window(signal_single_capture_replay_t *capture)
{
    const size_t block_bytes = (size_t)capture->config.samples_per_block *
        sizeof(*capture->config.search_buffer);
    memmove(capture->config.search_buffer,
        &capture->config.search_buffer[capture->config.samples_per_block],
        block_bytes);
}

signal_result_t SignalSingleCaptureReplay_Init(
    signal_single_capture_replay_t *capture,
    const signal_single_capture_replay_config_t *config)
{
    uint8_t slot;
    if ((capture == NULL) || !configuration_valid(config)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    memset(capture, 0, sizeof(*capture));
    capture->config = *config;
    for (slot = 0U; slot < config->slot_count; ++slot) {
        config->slot_lengths[slot] = 0U;
        config->slot_sample_rates_hz[slot] = 0U;
        config->slot_valid[slot] = false;
    }
    capture->initialized = true;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalSingleCaptureReplay_Arm(
    signal_single_capture_replay_t *capture)
{
    signal_result_t result;
    if ((capture == NULL) || !capture->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    SignalDualADC_Stop();
    result = SignalDualADC_SetSampleRate(
        capture->config.requested_sample_rate_hz);
    if (result != SIGNAL_RESULT_OK) return result;
    capture->effective_sample_rate_hz = SignalDualADC_GetConfiguredRate();
    capture->previous_block_valid = false;
    capture->trigger_pending = false;
    capture->pending_trigger_index = 0U;
    capture->trigger_seen = false;
    capture->dma_sequence = 0U;
    capture->selected_slot = capture->next_slot;
    if (capture->config.clear_trigger != NULL) {
        capture->config.clear_trigger(capture->config.trigger_context);
    }
    result = SignalDualADC_StartContinuous(capture->config.adc_channel_a,
        capture->config.adc_channel_b, capture->config.samples_per_block,
        capture->config.dma_block_count);
    if (result == SIGNAL_RESULT_OK) capture->armed = true;
    return result;
}

void SignalSingleCaptureReplay_Cancel(
    signal_single_capture_replay_t *capture)
{
    if ((capture == NULL) || !capture->initialized) return;
    if (capture->armed) SignalDualADC_Stop();
    capture->armed = false;
    capture->trigger_seen = false;
    capture->trigger_pending = false;
}

void SignalSingleCaptureReplay_NotifyTrigger(
    signal_single_capture_replay_t *capture)
{
    if ((capture != NULL) && capture->initialized && capture->armed) {
        capture->trigger_seen = true;
    }
}

signal_result_t SignalSingleCaptureReplay_Service(
    signal_single_capture_replay_t *capture)
{
    signal_trigger_config_t trigger;
    signal_result_t result;
    uint32_t sequence;
    uint8_t block;
    const uint16_t *completed;
    size_t trigger_index;
    uint16_t trimmed_length;
    uint16_t *selected_samples;
    size_t block_bytes;

    if ((capture == NULL) || !capture->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    if (!capture->armed) return SIGNAL_RESULT_NO_DATA;
    if (!SignalDualADC_GetContinuousSnapshot(&sequence, &block) ||
        (sequence == capture->dma_sequence) ||
        (block >= capture->config.dma_block_count)) {
        return SIGNAL_RESULT_NO_DATA;
    }
    capture->dma_sequence = sequence;
    if ((capture->config.consume_trigger != NULL) &&
        capture->config.consume_trigger(capture->config.trigger_context)) {
        capture->trigger_seen = true;
    }
    block_bytes = (size_t)capture->config.samples_per_block * sizeof(uint16_t);
    completed = &capture->config.adc_channel_a[(size_t)block *
        capture->config.samples_per_block];
    if (!capture->previous_block_valid) {
        memcpy(capture->config.search_buffer, completed, block_bytes);
        capture->previous_block_valid = true;
        return SIGNAL_RESULT_NO_DATA;
    }
    memcpy(&capture->config.search_buffer[capture->config.samples_per_block],
        completed, block_bytes);
    if (!capture->trigger_seen && !capture->trigger_pending) {
        roll_search_window(capture);
        return SIGNAL_RESULT_NO_DATA;
    }
    trigger.level = capture->config.trigger_level_code;
    trigger.hysteresis = capture->config.trigger_hysteresis_code;
    trigger.edge = capture->config.trigger_edge;
    if (capture->trigger_pending) {
        trigger_index = capture->pending_trigger_index;
        capture->trigger_pending = false;
        result = SIGNAL_RESULT_OK;
    } else {
        result = SignalTrigger_Find(capture->config.search_buffer,
            (size_t)capture->config.samples_per_block * 2U, &trigger,
            capture->config.pretrigger_samples - 1U, &trigger_index);
    }
    selected_samples = slot_pointer(capture, capture->selected_slot);
    if (result == SIGNAL_RESULT_OK) {
        if ((trigger_index >= capture->config.pretrigger_samples) &&
            (trigger_index + capture->config.samples_per_block -
                capture->config.pretrigger_samples <=
                (size_t)capture->config.samples_per_block * 2U)) {
            result = SignalTrigger_Extract(capture->config.search_buffer,
                (size_t)capture->config.samples_per_block * 2U,
                trigger_index, capture->config.pretrigger_samples,
                selected_samples, capture->config.samples_per_block);
        } else if (trigger_index >= capture->config.samples_per_block) {
            capture->trigger_pending = true;
            capture->pending_trigger_index = (uint16_t)(trigger_index -
                capture->config.samples_per_block);
            result = SIGNAL_RESULT_NO_DATA;
        } else {
            result = SIGNAL_RESULT_NO_DATA;
        }
    }
    if (result == SIGNAL_RESULT_OK) {
        trimmed_length = trim_baseline(capture, selected_samples);
        if (trimmed_length >= 2U) {
            SignalDualADC_Stop();
            capture->config.slot_lengths[capture->selected_slot] =
                trimmed_length;
            capture->config.slot_sample_rates_hz[capture->selected_slot] =
                capture->effective_sample_rate_hz;
            capture->config.slot_valid[capture->selected_slot] = true;
            capture->next_slot = (uint8_t)((capture->selected_slot + 1U) %
                capture->config.slot_count);
            capture->trigger_seen = false;
            capture->trigger_pending = false;
            capture->armed = false;
            return SIGNAL_RESULT_OK;
        }
        capture->trigger_seen = false;
        capture->trigger_pending = false;
    }
    roll_search_window(capture);
    return SIGNAL_RESULT_NO_DATA;
}

bool SignalSingleCaptureReplay_IsArmed(
    const signal_single_capture_replay_t *capture)
{
    return (capture != NULL) && capture->initialized && capture->armed;
}

signal_result_t SignalSingleCaptureReplay_SelectSlot(
    signal_single_capture_replay_t *capture, uint8_t slot_index)
{
    if ((capture == NULL) || !capture->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    if (slot_index >= capture->config.slot_count) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    capture->selected_slot = slot_index;
    return SIGNAL_RESULT_OK;
}

uint8_t SignalSingleCaptureReplay_GetSelectedSlot(
    const signal_single_capture_replay_t *capture)
{
    return ((capture != NULL) && capture->initialized) ?
        capture->selected_slot : 0U;
}

uint8_t SignalSingleCaptureReplay_GetNextSlot(
    const signal_single_capture_replay_t *capture)
{
    return ((capture != NULL) && capture->initialized) ?
        capture->next_slot : 0U;
}

signal_result_t SignalSingleCaptureReplay_GetSelected(
    const signal_single_capture_replay_t *capture,
    const uint16_t **samples, signal_single_capture_info_t *info)
{
    uint8_t slot;
    uint16_t count;
    uint16_t index;
    if ((capture == NULL) || !capture->initialized || (samples == NULL) ||
        (info == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    slot = capture->selected_slot;
    if (!capture->config.slot_valid[slot]) return SIGNAL_RESULT_NO_DATA;
    count = capture->config.slot_lengths[slot];
    if ((count < 2U) || (count > capture->config.samples_per_block) ||
        (capture->config.slot_sample_rates_hz[slot] == 0U)) {
        return SIGNAL_RESULT_NO_DATA;
    }
    *samples = const_slot_pointer(capture, slot);
    info->sample_count = count;
    info->sample_rate_hz = capture->config.slot_sample_rates_hz[slot];
    info->duration_us = ((uint32_t)count * 1000000U +
        info->sample_rate_hz / 2U) / info->sample_rate_hz;
    info->minimum_code = (*samples)[0];
    info->maximum_code = (*samples)[0];
    for (index = 1U; index < count; ++index) {
        if ((*samples)[index] < info->minimum_code) {
            info->minimum_code = (*samples)[index];
        }
        if ((*samples)[index] > info->maximum_code) {
            info->maximum_code = (*samples)[index];
        }
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalSingleCaptureReplay_ReplaySelected(
    signal_single_capture_replay_t *capture)
{
    const uint16_t *samples;
    signal_single_capture_info_t info;
    size_t replay_count;
    uint32_t replay_rate_hz;
    signal_result_t result;
    if ((capture == NULL) || !capture->initialized) {
        return SIGNAL_RESULT_NOT_INITIALIZED;
    }
    result = SignalSingleCaptureReplay_GetSelected(capture, &samples, &info);
    if (result != SIGNAL_RESULT_OK) return result;
    replay_count = (info.sample_count > capture->config.replay_table_capacity) ?
        capture->config.replay_table_capacity : info.sample_count;
    replay_rate_hz = ((uint32_t)replay_count * info.sample_rate_hz +
        info.sample_count / 2U) / info.sample_count;
    SignalDACDMA_MSPM0_Stop();
    result = SignalDACDMA_MSPM0_SetUpdateRate(replay_rate_hz);
    if (result != SIGNAL_RESULT_OK) return result;
    result = SignalArbitraryWave_ResampleLinear(samples, info.sample_count,
        capture->config.replay_table, replay_count);
    if (result != SIGNAL_RESULT_OK) return result;
    return SignalDACDMA_MSPM0_Start(capture->config.replay_table,
        replay_count, true);
}

signal_result_t SignalSingleCaptureReplay_DrawSelectedST7789(
    const signal_single_capture_replay_t *capture, tft_st7789_t *tft,
    const signal_single_capture_plot_config_t *plot,
    signal_single_capture_info_t *info)
{
    const uint16_t *samples;
    signal_single_capture_info_t local_info;
    uint16_t minimum;
    uint16_t maximum;
    uint16_t padding;
    uint32_t span;
    int32_t column;
    int32_t previous_x = 0;
    int32_t previous_y = 0;
    bool previous_valid = false;
    signal_result_t result;

    if ((capture == NULL) || (tft == NULL) || (plot == NULL) ||
        (plot->width <= 0) || (plot->height <= 0) || (plot->x < 0) ||
        (plot->y < 0) || (plot->x + plot->width > tft->width) ||
        (plot->y + plot->height > tft->height)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    result = SignalSingleCaptureReplay_GetSelected(capture, &samples,
        &local_info);
    if (result != SIGNAL_RESULT_OK) return result;
    if (info != NULL) *info = local_info;
    minimum = local_info.minimum_code;
    maximum = local_info.maximum_code;
    padding = (uint16_t)(((uint32_t)(maximum - minimum) + 19U) / 20U);
    if (padding < 4U) padding = 4U;
    minimum = (minimum > padding) ? (uint16_t)(minimum - padding) : 0U;
    maximum = ((uint32_t)maximum + padding < capture->config.adc_max_code) ?
        (uint16_t)(maximum + padding) : capture->config.adc_max_code;
    span = (maximum > minimum) ? (uint32_t)(maximum - minimum) : 1U;
    if (plot->clear_background &&
        (TFT_ST7789_FillRect(tft, plot->x, plot->y, plot->width,
            plot->height, plot->background_color) != TFT_ST7789_OK)) {
        return SIGNAL_RESULT_HARDWARE_ERROR;
    }
    for (column = 0; column < plot->width; ++column) {
        size_t sample_index = (plot->width > 1) ?
            ((size_t)column * (local_info.sample_count - 1U)) /
                (size_t)(plot->width - 1) : 0U;
        uint32_t value = (samples[sample_index] > minimum) ?
            (uint32_t)(samples[sample_index] - minimum) : 0U;
        int32_t y;
        if (value > span) value = span;
        y = plot->y + plot->height - 1 -
            (int32_t)(value * (uint32_t)(plot->height - 1) / span);
        if (y < plot->y) y = plot->y;
        if (y >= plot->y + plot->height) y = plot->y + plot->height - 1;
        if (previous_valid) {
            if (TFT_ST7789_DrawLine(tft, previous_x, previous_y,
                    plot->x + column, y, plot->waveform_color) !=
                TFT_ST7789_OK) {
                return SIGNAL_RESULT_HARDWARE_ERROR;
            }
        } else if (TFT_ST7789_DrawLine(tft, plot->x + column, y,
                plot->x + column, y, plot->waveform_color) !=
            TFT_ST7789_OK) {
            return SIGNAL_RESULT_HARDWARE_ERROR;
        }
        previous_x = plot->x + column;
        previous_y = y;
        previous_valid = true;
    }
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalSingleCaptureReplay_GetModuleStatus(void)
{
    return MODULE_STATUS_BOARD_VERIFIED;
}
