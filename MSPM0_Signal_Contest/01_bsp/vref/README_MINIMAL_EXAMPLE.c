/* vref 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_vref.h"

void vref_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_vref_calibration_t signalvref_geteffectivevoltage_arg0 = {0};
    static float signalvref_geteffectivevoltage_arg1[16] = {0};
    /* ===== 最小入口：SignalVREF_GetEffectiveVoltage ===== */
    (void)SignalVREF_GetEffectiveVoltage(&signalvref_geteffectivevoltage_arg0, signalvref_geteffectivevoltage_arg1);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

