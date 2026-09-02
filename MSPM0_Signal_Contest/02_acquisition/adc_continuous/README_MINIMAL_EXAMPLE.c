/* 最小示例：把一帧已经采好的 ADC 数据交给自己的处理函数。 */
#include <stddef.h>
#include <stdint.h>

#include "signal_adc_continuous.h"

/* 这里通常改成上游 ADC 每帧实际给出的样本数。 */
#define FRAME_SAMPLE_COUNT (8U)

static signal_adc_continuous_t g_dispatcher;

/* 回调内只做本帧必须的工作；samples 在回调返回后可能被上游复用。 */
static void ProcessFrame(void *context, const uint16_t *samples,
    size_t count, uint32_t sequence)
{
    uint32_t *last_sequence = (uint32_t *)context;
    (void)samples;
    (void)count;
    *last_sequence = sequence;
    /* ===== 从这里开始写自己的 RMS、FFT、显示等处理 ===== */
}

void SignalADCContinuous_MinimalExample(void)
{
    /* 实际工程中该数组来自已经完成的 ADC/DMA 帧。 */
    static const uint16_t raw[FRAME_SAMPLE_COUNT] = {
        2010U, 2020U, 2030U, 2040U, 2050U, 2060U, 2070U, 2080U
    };
    static uint32_t last_sequence;

    if (SignalADCContinuous_Init(&g_dispatcher, ProcessFrame,
            &last_sequence) != SIGNAL_RESULT_OK) {
        return;
    }
    if (SignalADCContinuous_Start(&g_dispatcher) != SIGNAL_RESULT_OK) {
        return;
    }

    /* 上游确认这一帧不再被 DMA 写入后，才提交。 */
    (void)SignalADCContinuous_SubmitFrame(&g_dispatcher, raw,
        FRAME_SAMPLE_COUNT);
}
