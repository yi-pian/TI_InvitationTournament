/* jacobsen_interpolation 最小示例：先完成一个最短、可读的正常调用流程。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_jacobsen_interpolation.h"

void jacobsen_interpolation_MinimalExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_complex_f32_t signaljacobsen_process_arg0 = {0};
    static uint32_t signaljacobsen_process_arg1 = 0U;
    static uint32_t signaljacobsen_process_arg2 = 0U;
    static float signaljacobsen_process_arg3 = 0.0f;
    static uint32_t signaljacobsen_process_arg4 = 0U;
    static signal_jacobsen_result_t signaljacobsen_process_arg5 = {0};
    /* ===== 最小入口：SignalJacobsen_Process ===== */
    (void)SignalJacobsen_Process(&signaljacobsen_process_arg0, signaljacobsen_process_arg1, signaljacobsen_process_arg2, signaljacobsen_process_arg3, signaljacobsen_process_arg4, &signaljacobsen_process_arg5);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

