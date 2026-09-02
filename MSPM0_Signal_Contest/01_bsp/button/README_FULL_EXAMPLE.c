/* button 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_button.h"

void button_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_button_t signalbutton_init_arg0 = {0};
    static signal_button_config_t signalbutton_init_arg1 = {0};
    /* ===== 调用 SignalButton_Init：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalButton_Init(&signalbutton_init_arg0, &signalbutton_init_arg1);

    static signal_button_t signalbutton_update_arg0 = {0};
    static signal_button_event_t signalbutton_update_arg1 = {0};
    /* ===== 调用 SignalButton_Update：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalButton_Update(&signalbutton_update_arg0, &signalbutton_update_arg1);

    /* ===== 调用 SignalButton_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalButton_GetModuleStatus();

    static signal_button_t signalbutton_getpressed_arg0 = {0};
    static bool signalbutton_getpressed_arg1[16] = {0};
    /* ===== 调用 SignalButton_GetPressed：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalButton_GetPressed(&signalbutton_getpressed_arg0, signalbutton_getpressed_arg1);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

