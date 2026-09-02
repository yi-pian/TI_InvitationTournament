/* arbitrary_wave 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_arbitrary_wave.h"

void arbitrary_wave_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    /* ===== 调用 SignalArbitraryWave_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalArbitraryWave_GetModuleStatus();

    static uint16_t signalarbitrarywave_resamplelinear_arg0[16] = {0};
    static size_t signalarbitrarywave_resamplelinear_arg1 = 0U;
    static uint16_t signalarbitrarywave_resamplelinear_arg2[16] = {0};
    static size_t signalarbitrarywave_resamplelinear_arg3 = 0U;
    /* ===== 调用 SignalArbitraryWave_ResampleLinear：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalArbitraryWave_ResampleLinear(signalarbitrarywave_resamplelinear_arg0, signalarbitrarywave_resamplelinear_arg1, signalarbitrarywave_resamplelinear_arg2, signalarbitrarywave_resamplelinear_arg3);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

