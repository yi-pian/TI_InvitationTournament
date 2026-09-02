/* snr 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_snr.h"

void snr_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signalsnr_process_arg0[16] = {0};
    static uint32_t signalsnr_process_arg1 = 0U;
    static signal_snr_config_t signalsnr_process_arg2 = {0};
    static signal_snr_result_t signalsnr_process_arg3 = {0};
    /* ===== 调用 SignalSNR_Process：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalSNR_Process(signalsnr_process_arg0, signalsnr_process_arg1, &signalsnr_process_arg2, &signalsnr_process_arg3);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

