/* dds 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_dds.h"

void dds_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_dds_t signaldds_init_arg0 = {0};
    static uint16_t signaldds_init_arg1[16] = {0};
    static size_t signaldds_init_arg2 = 0U;
    static float signaldds_init_arg3 = 0.0f;
    static float signaldds_init_arg4 = 0.0f;
    static uint32_t signaldds_init_arg5 = 0U;
    /* ===== 调用 SignalDDS_Init：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDDS_Init(&signaldds_init_arg0, signaldds_init_arg1, signaldds_init_arg2, signaldds_init_arg3, signaldds_init_arg4, signaldds_init_arg5);

    static signal_dds_t signaldds_setfrequency_arg0 = {0};
    static float signaldds_setfrequency_arg1 = 0.0f;
    static float signaldds_setfrequency_arg2 = 0.0f;
    /* ===== 调用 SignalDDS_SetFrequency：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDDS_SetFrequency(&signaldds_setfrequency_arg0, signaldds_setfrequency_arg1, signaldds_setfrequency_arg2);

    static signal_dds_t signaldds_getconfiguredfrequency_arg0 = {0};
    static float signaldds_getconfiguredfrequency_arg1 = 0.0f;
    /* ===== 调用 SignalDDS_GetConfiguredFrequency：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDDS_GetConfiguredFrequency(&signaldds_getconfiguredfrequency_arg0, signaldds_getconfiguredfrequency_arg1);

    /* ===== 调用 SignalDDS_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDDS_GetModuleStatus();

    static signal_dds_t signaldds_fill_arg0 = {0};
    static uint16_t signaldds_fill_arg1[16] = {0};
    static size_t signaldds_fill_arg2 = 0U;
    /* ===== 调用 SignalDDS_Fill：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDDS_Fill(&signaldds_fill_arg0, signaldds_fill_arg1, signaldds_fill_arg2);

    static signal_dds_t signaldds_next_arg0 = {0};
    /* ===== 调用 SignalDDS_Next：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDDS_Next(&signaldds_next_arg0);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

