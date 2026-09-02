/* sine 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_sine.h"

void sine_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_dac_wave_table_t signalsine_generate_arg0 = {0};
    static float signalsine_generate_arg1 = 0.0f;
    static float signalsine_generate_arg2 = 0.0f;
    static float signalsine_generate_arg3 = 0.0f;
    /* ===== 最小入口：SignalSine_Generate ===== */
    (void)SignalSine_Generate(&signalsine_generate_arg0, signalsine_generate_arg1, signalsine_generate_arg2, signalsine_generate_arg3);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

