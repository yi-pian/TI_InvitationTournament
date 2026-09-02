/* square 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_square.h"

void square_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_dac_wave_table_t signalsquare_generate_arg0 = {0};
    static float signalsquare_generate_arg1 = 0.0f;
    static float signalsquare_generate_arg2 = 0.0f;
    static float signalsquare_generate_arg4 = 0.0f;
    /* ===== 调用 SignalSquare_GenerateWithDuty：占空比是 0～1 小数 ===== */
    (void)SignalSquare_GenerateWithDuty(&signalsquare_generate_arg0, signalsquare_generate_arg1, signalsquare_generate_arg2, 0.5f, signalsquare_generate_arg4);
    /* 旧 API 仍保留，传入 duty_fraction 也能兼容已有工程。 */
    (void)SignalSquare_Generate(&signalsquare_generate_arg0, signalsquare_generate_arg1, signalsquare_generate_arg2, 0.5f, signalsquare_generate_arg4);

    /* ===== 调用 SignalSquare_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalSquare_GetModuleStatus();

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

