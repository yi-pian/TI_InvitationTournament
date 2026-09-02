/* mad 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_mad.h"

void mad_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalmad_process_arg0[16] = {0};
    static uint32_t signalmad_process_arg1 = 0U;
    static float signalmad_process_arg2[16] = {0};
    static uint32_t signalmad_process_arg3 = 0U;
    static signal_mad_result_t signalmad_process_arg4 = {0};
    /* ===== 最小入口：SignalMAD_Process ===== */
    (void)SignalMAD_Process(signalmad_process_arg0, signalmad_process_arg1, signalmad_process_arg2, signalmad_process_arg3, &signalmad_process_arg4);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

