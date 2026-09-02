/* square 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_square.h"

void square_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_dac_wave_table_t signalsquare_generate_arg0 = {0};
    static float signalsquare_generate_arg1 = 0.0f;
    static float signalsquare_generate_arg2 = 0.0f;
    static float signalsquare_generate_arg4 = 0.0f;
    /* ===== 最小入口：SignalSquare_GenerateWithDuty，0.5f 表示 50% ===== */
    (void)SignalSquare_GenerateWithDuty(&signalsquare_generate_arg0, signalsquare_generate_arg1, signalsquare_generate_arg2, 0.5f, signalsquare_generate_arg4);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

