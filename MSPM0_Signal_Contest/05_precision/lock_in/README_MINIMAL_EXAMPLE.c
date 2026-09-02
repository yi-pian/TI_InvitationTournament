/* lock_in 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_lock_in.h"

void lock_in_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signallockin_process_arg0[16] = {0};
    static uint32_t signallockin_process_arg1 = 0U;
    static signal_lock_in_config_t signallockin_process_arg2 = {0};
    static signal_lock_in_result_t signallockin_process_arg3 = {0};
    /* ===== 最小入口：SignalLockIn_Process ===== */
    (void)SignalLockIn_Process(signallockin_process_arg0, signallockin_process_arg1, &signallockin_process_arg2, &signallockin_process_arg3);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

