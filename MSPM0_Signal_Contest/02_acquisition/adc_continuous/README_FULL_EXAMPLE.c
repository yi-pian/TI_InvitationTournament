/* 全功能示例：展示连续帧分发器的全部公开 API。 */
#include <stddef.h>
#include <stdint.h>

#include "signal_adc_continuous.h"

#define FRAME_SAMPLE_COUNT (8U) /* 每帧样本数，应与上游采集帧一致。 */

static signal_adc_continuous_t g_dispatcher;
static uint32_t g_last_sequence;

static void ProcessFrame(void *context, const uint16_t *samples,
    size_t count, uint32_t sequence)
{
    uint32_t *last_sequence = (uint32_t *)context;
    (void)samples;
    (void)count;
    *last_sequence = sequence;
}

void SignalADCContinuous_FullExample(void)
{
    static const uint16_t first_frame[FRAME_SAMPLE_COUNT] = {
        2000U, 2010U, 2020U, 2030U, 2040U, 2050U, 2060U, 2070U
    };
    signal_result_t result;
    signal_module_status_t maturity;

    result = SignalADCContinuous_Init(&g_dispatcher, ProcessFrame,
        &g_last_sequence);
    if (result != SIGNAL_RESULT_OK) {
        return;
    }

    maturity = SignalADCContinuous_GetModuleStatus();
    (void)maturity; /* 证据等级用于界面或日志，不是实时运行状态。 */

    result = SignalADCContinuous_Start(&g_dispatcher);
    if (result == SIGNAL_RESULT_OK) {
        result = SignalADCContinuous_SubmitFrame(&g_dispatcher, first_frame,
            FRAME_SAMPLE_COUNT);
    }

    /* Stop 后的 SubmitFrame 返回 BUSY，并把 dropped_frames 加一。 */
    (void)SignalADCContinuous_Stop(&g_dispatcher);
#if 0
    /* 仅演示错误路径：默认不要向已停止的分发器提交有效采集帧。 */
    (void)SignalADCContinuous_SubmitFrame(&g_dispatcher, first_frame,
        FRAME_SAMPLE_COUNT);
#endif
    (void)result;
}
