/* adc_dual_sync 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_adc_dual_sync.h"

void adc_dual_sync_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    /* ===== 调用 SignalADCDualSync_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADCDualSync_GetModuleStatus();

    static uint16_t signaladcdualsync_deinterleave_arg0[16] = {0};
    static size_t signaladcdualsync_deinterleave_arg1 = 0U;
    static uint16_t signaladcdualsync_deinterleave_arg2[16] = {0};
    static size_t signaladcdualsync_deinterleave_arg3 = 0U;
    static uint16_t signaladcdualsync_deinterleave_arg4[16] = {0};
    static size_t signaladcdualsync_deinterleave_arg5 = 0U;
    /* ===== 调用 SignalADCDualSync_Deinterleave：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADCDualSync_Deinterleave(signaladcdualsync_deinterleave_arg0, signaladcdualsync_deinterleave_arg1, signaladcdualsync_deinterleave_arg2, signaladcdualsync_deinterleave_arg3, signaladcdualsync_deinterleave_arg4, signaladcdualsync_deinterleave_arg5);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

