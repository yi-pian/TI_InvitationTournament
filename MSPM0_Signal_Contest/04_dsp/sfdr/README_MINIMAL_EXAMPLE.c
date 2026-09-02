/* sfdr 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_sfdr.h"

void sfdr_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalsfdr_process_arg0[16] = {0};
    static uint32_t signalsfdr_process_arg1 = 0U;
    static signal_sfdr_config_t signalsfdr_process_arg2 = {0};
    static signal_sfdr_result_t signalsfdr_process_arg3 = {0};
    /* ===== 最小入口：SignalSFDR_Process ===== */
    (void)SignalSFDR_Process(signalsfdr_process_arg0, signalsfdr_process_arg1, &signalsfdr_process_arg2, &signalsfdr_process_arg3);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

