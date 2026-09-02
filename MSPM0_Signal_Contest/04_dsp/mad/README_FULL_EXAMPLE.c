/* mad 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_mad.h"

void mad_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalmad_process_arg0[16] = {0};
    static uint32_t signalmad_process_arg1 = 0U;
    static float signalmad_process_arg2[16] = {0};
    static uint32_t signalmad_process_arg3 = 0U;
    static signal_mad_result_t signalmad_process_arg4 = {0};
    /* ===== 调用 SignalMAD_Process：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalMAD_Process(signalmad_process_arg0, signalmad_process_arg1, signalmad_process_arg2, signalmad_process_arg3, &signalmad_process_arg4);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

