/* vref 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_vref.h"

void vref_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_vref_calibration_t signalvref_geteffectivevoltage_arg0 = {0};
    static float signalvref_geteffectivevoltage_arg1[16] = {0};
    /* ===== 调用 SignalVREF_GetEffectiveVoltage：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalVREF_GetEffectiveVoltage(&signalvref_geteffectivevoltage_arg0, signalvref_geteffectivevoltage_arg1);

    /* ===== 调用 SignalVREF_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalVREF_GetModuleStatus();

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

