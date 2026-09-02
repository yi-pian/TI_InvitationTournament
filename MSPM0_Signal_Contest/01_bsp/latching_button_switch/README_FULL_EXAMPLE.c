/* latching_button_switch 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_latching_button_switch.h"

void latching_button_switch_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_latching_button_switch_t signallatchingbuttonswitch_init_arg0 = {0};
    static signal_latching_button_switch_config_t signallatchingbuttonswitch_init_arg1 = {0};
    /* ===== 调用 SignalLatchingButtonSwitch_Init：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalLatchingButtonSwitch_Init(&signallatchingbuttonswitch_init_arg0, &signallatchingbuttonswitch_init_arg1);

    static signal_latching_button_switch_t signallatchingbuttonswitch_update_arg0 = {0};
    static signal_latching_button_switch_event_t signallatchingbuttonswitch_update_arg1 = {0};
    /* ===== 调用 SignalLatchingButtonSwitch_Update：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalLatchingButtonSwitch_Update(&signallatchingbuttonswitch_update_arg0, &signallatchingbuttonswitch_update_arg1);

    /* ===== 调用 SignalLatchingButtonSwitch_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalLatchingButtonSwitch_GetModuleStatus();

    static signal_latching_button_switch_t signallatchingbuttonswitch_getstate_arg0 = {0};
    static bool signallatchingbuttonswitch_getstate_arg1[16] = {0};
    /* ===== 调用 SignalLatchingButtonSwitch_GetState：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalLatchingButtonSwitch_GetState(&signallatchingbuttonswitch_getstate_arg0, signallatchingbuttonswitch_getstate_arg1);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

