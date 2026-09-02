#ifndef SIGNAL_ADC_CONTINUOUS_H
#define SIGNAL_ADC_CONTINUOUS_H

#include <stddef.h>
#include <stdint.h>
#include "signal_status.h"

/**
 * @brief 一帧 ADC 数据准备好时由模块调用的业务处理函数。
 *
 * 回调运行在调用 SignalADCContinuous_SubmitFrame() 的上下文中，而不是
 * 模块自行创建的中断中。回调返回后，调用者可以复用 samples 所指的缓冲区，
 * 因此若需长期保存数据，必须在回调内自行复制。
 *
 * @param context Init 时保存的用户上下文指针，可为 NULL。
 * @param samples 本帧只读 ADC 原始码数组，不能为空。
 * @param count 本帧样本数，保证非零。
 * @param sequence 从 1 开始的成功提交帧号。
 */
typedef void (*signal_adc_frame_callback_t)(void *context,
    const uint16_t *samples, size_t count, uint32_t sequence);

/** @brief 连续帧分发器的调用者持有状态。请作为长期对象保存，不要每帧清零。 */
typedef struct {
    signal_adc_frame_callback_t callback;
    void *callback_context;
    uint32_t completed_frames;
    uint32_t dropped_frames;
    signal_status_t state;
} signal_adc_continuous_t;

/**
 * @brief 绑定业务回调并清零完成帧、丢帧计数。
 * @param module 调用者分配的模块状态，不能为空。
 * @param callback 每一帧的处理函数，不能为空。
 * @param callback_context 原样传回 callback 的用户指针，可为 NULL。
 * @return OK 成功；INVALID_ARGUMENT 表示 module 或 callback 为空。
 */
signal_result_t SignalADCContinuous_Init(signal_adc_continuous_t *module,
    signal_adc_frame_callback_t callback, void *callback_context);
/** @brief 允许接收帧。重复 Start 返回 BUSY；未正确 Init 返回 NOT_INITIALIZED。 */
signal_result_t SignalADCContinuous_Start(signal_adc_continuous_t *module);
/** @brief 停止接收帧。之后提交的帧会计入 dropped_frames。 */
signal_result_t SignalADCContinuous_Stop(signal_adc_continuous_t *module);
/**
 * @brief 把上游已完成的一帧数据交给 callback。
 * @param module 已 Init 且已 Start 的模块状态。
 * @param samples 上游拥有的只读样本数组，不能为空。
 * @param count 样本元素数，必须非零。
 * @return OK 已调用 callback；BUSY 表示未 Start 且本帧已丢弃；
 *         INVALID_ARGUMENT 表示指针或 count 非法。
 */
signal_result_t SignalADCContinuous_SubmitFrame(signal_adc_continuous_t *module,
    const uint16_t *samples, size_t count);
/** @brief 返回本模块当前构建证据等级，不是运行时状态。 */
signal_module_status_t SignalADCContinuous_GetModuleStatus(void);

#endif
