/* triangle 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_triangle.h"

void triangle_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_dac_wave_table_t signaltriangle_generate_arg0 = {0};
    static float signaltriangle_generate_arg1 = 0.0f;
    static float signaltriangle_generate_arg2 = 0.0f;
    static float signaltriangle_generate_arg3 = 0.0f;
    /* ===== 最小入口：SignalTriangle_Generate ===== */
    (void)SignalTriangle_Generate(&signaltriangle_generate_arg0, signaltriangle_generate_arg1, signaltriangle_generate_arg2, signaltriangle_generate_arg3);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

