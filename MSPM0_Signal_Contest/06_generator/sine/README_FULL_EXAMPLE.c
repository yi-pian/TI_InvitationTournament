/* sine 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_sine.h"

void sine_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_dac_wave_table_t signalsine_generate_arg0 = {0};
    static float signalsine_generate_arg1 = 0.0f;
    static float signalsine_generate_arg2 = 0.0f;
    static float signalsine_generate_arg3 = 0.0f;
    /* ===== 调用 SignalSine_Generate：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalSine_Generate(&signalsine_generate_arg0, signalsine_generate_arg1, signalsine_generate_arg2, signalsine_generate_arg3);

    /* ===== 调用 SignalSine_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalSine_GetModuleStatus();

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

