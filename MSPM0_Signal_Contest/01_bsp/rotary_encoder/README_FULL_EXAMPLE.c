/* rotary_encoder 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_rotary_encoder.h"

void rotary_encoder_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_rotary_encoder_t signalrotaryencoder_init_arg0 = {0};
    static signal_rotary_encoder_config_t signalrotaryencoder_init_arg1 = {0};
    /* ===== 调用 SignalRotaryEncoder_Init：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalRotaryEncoder_Init(&signalrotaryencoder_init_arg0, &signalrotaryencoder_init_arg1);

    static signal_rotary_encoder_t signalrotaryencoder_setposition_arg0 = {0};
    static int32_t signalrotaryencoder_setposition_arg1 = 0U;
    /* ===== 调用 SignalRotaryEncoder_SetPosition：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalRotaryEncoder_SetPosition(&signalrotaryencoder_setposition_arg0, signalrotaryencoder_setposition_arg1);

    static signal_rotary_encoder_t signalrotaryencoder_update_arg0 = {0};
    static signal_rotary_encoder_event_t signalrotaryencoder_update_arg1 = {0};
    /* ===== 调用 SignalRotaryEncoder_Update：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalRotaryEncoder_Update(&signalrotaryencoder_update_arg0, &signalrotaryencoder_update_arg1);

    static signal_rotary_encoder_t signalrotaryencoder_getinvalidtransitioncount_arg0 = {0};
    static uint32_t signalrotaryencoder_getinvalidtransitioncount_arg1[16] = {0};
    /* ===== 调用 SignalRotaryEncoder_GetInvalidTransitionCount：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalRotaryEncoder_GetInvalidTransitionCount(&signalrotaryencoder_getinvalidtransitioncount_arg0, signalrotaryencoder_getinvalidtransitioncount_arg1);

    /* ===== 调用 SignalRotaryEncoder_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalRotaryEncoder_GetModuleStatus();

    static signal_rotary_encoder_t signalrotaryencoder_getposition_arg0 = {0};
    static int32_t signalrotaryencoder_getposition_arg1[16] = {0};
    /* ===== 调用 SignalRotaryEncoder_GetPosition：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalRotaryEncoder_GetPosition(&signalrotaryencoder_getposition_arg0, signalrotaryencoder_getposition_arg1);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

