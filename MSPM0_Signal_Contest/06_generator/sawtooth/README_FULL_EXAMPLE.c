/* sawtooth 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_sawtooth.h"

void sawtooth_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_dac_wave_table_t signalsawtooth_generate_arg0 = {0};
    static float signalsawtooth_generate_arg1 = 0.0f;
    static float signalsawtooth_generate_arg2 = 0.0f;
    static float signalsawtooth_generate_arg3 = 0.0f;
    static bool signalsawtooth_generate_arg4 = false;
    /* ===== 调用 SignalSawtooth_GenerateWithSymmetry：1.0f 是 100% 标准上升锯齿 ===== */
    (void)SignalSawtooth_GenerateWithSymmetry(&signalsawtooth_generate_arg0, signalsawtooth_generate_arg1, signalsawtooth_generate_arg2, signalsawtooth_generate_arg3, signalsawtooth_generate_arg4, 1.0f);
    /* 旧 API 保留并固定 50% 对称度。 */
    (void)SignalSawtooth_Generate(&signalsawtooth_generate_arg0, signalsawtooth_generate_arg1, signalsawtooth_generate_arg2, signalsawtooth_generate_arg3, signalsawtooth_generate_arg4);

    /* ===== 调用 SignalSawtooth_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalSawtooth_GetModuleStatus();

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

