/* czt 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_czt.h"

void czt_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalczt_unitcirclerealdirect_arg0[16] = {0};
    static uint32_t signalczt_unitcirclerealdirect_arg1 = 0U;
    static float signalczt_unitcirclerealdirect_arg2 = 0.0f;
    static float signalczt_unitcirclerealdirect_arg3 = 0.0f;
    static float signalczt_unitcirclerealdirect_arg4 = 0.0f;
    static signal_complex_f32_t signalczt_unitcirclerealdirect_arg5 = {0};
    static uint32_t signalczt_unitcirclerealdirect_arg6 = 0U;
    static uint32_t signalczt_unitcirclerealdirect_arg7 = 0U;
    /* ===== 调用 SignalCZT_UnitCircleRealDirect：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalCZT_UnitCircleRealDirect(signalczt_unitcirclerealdirect_arg0, signalczt_unitcirclerealdirect_arg1, signalczt_unitcirclerealdirect_arg2, signalczt_unitcirclerealdirect_arg3, signalczt_unitcirclerealdirect_arg4, &signalczt_unitcirclerealdirect_arg5, signalczt_unitcirclerealdirect_arg6, signalczt_unitcirclerealdirect_arg7);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

