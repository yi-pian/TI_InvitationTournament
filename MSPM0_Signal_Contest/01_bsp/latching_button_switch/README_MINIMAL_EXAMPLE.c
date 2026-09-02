/* latching_button_switch 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_latching_button_switch.h"

void latching_button_switch_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    /* ===== 最小入口：SignalLatchingButtonSwitch_GetModuleStatus ===== */
    (void)SignalLatchingButtonSwitch_GetModuleStatus();

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

