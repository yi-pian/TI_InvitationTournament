/* 全功能示例：展示初始化、武装、ISR通知、服务、选槽、取结果、显示和回放。 */
#include "signal_single_capture_replay.h"

void SignalSingleCaptureReplay_FullExample(
    signal_single_capture_replay_t *capture, tft_st7789_t *tft)
{
    const uint16_t *samples;
    signal_single_capture_info_t info;
    const signal_single_capture_plot_config_t plot = {
        .x = 8, .y = 112, .width = 304, .height = 90,
        .waveform_color = TFT_ST7789_YELLOW,
        .background_color = TFT_ST7789_BLACK,
        .clear_background = true
    };

    if (SignalSingleCaptureReplay_GetModuleStatus() <
        MODULE_STATUS_BUILD_VERIFIED) return;
    if (SignalSingleCaptureReplay_Arm(capture) != SIGNAL_RESULT_OK) return;

    /* GROUP1_IRQHandler 中清除 COMP 标志后调用。 */
    SignalSingleCaptureReplay_NotifyTrigger(capture);

    if (SignalSingleCaptureReplay_Service(capture) == SIGNAL_RESULT_OK) {
        (void)SignalSingleCaptureReplay_GetSelected(capture, &samples, &info);
        (void)samples;
        (void)SignalSingleCaptureReplay_DrawSelectedST7789(
            capture, tft, &plot, &info);
        (void)SignalSingleCaptureReplay_ReplaySelected(capture);
    }
    (void)SignalSingleCaptureReplay_SelectSlot(capture, 0U);
    (void)SignalSingleCaptureReplay_GetSelectedSlot(capture);
    (void)SignalSingleCaptureReplay_GetNextSlot(capture);
    (void)SignalSingleCaptureReplay_IsArmed(capture);
    SignalSingleCaptureReplay_Cancel(capture);
}
