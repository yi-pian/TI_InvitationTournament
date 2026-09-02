/* multi_bin_energy 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_multi_bin_energy.h"

void multi_bin_energy_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalmultibinenergy_process_arg0[16] = {0};
    static uint32_t signalmultibinenergy_process_arg1 = 0U;
    static uint32_t signalmultibinenergy_process_arg2 = 0U;
    static uint32_t signalmultibinenergy_process_arg3 = 0U;
    static signal_multi_bin_energy_result_t signalmultibinenergy_process_arg4 = {0};
    /* ===== 调用 SignalMultiBinEnergy_Process：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalMultiBinEnergy_Process(signalmultibinenergy_process_arg0, signalmultibinenergy_process_arg1, signalmultibinenergy_process_arg2, signalmultibinenergy_process_arg3, &signalmultibinenergy_process_arg4);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

