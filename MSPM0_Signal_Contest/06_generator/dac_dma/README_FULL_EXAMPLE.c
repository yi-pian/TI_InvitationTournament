/* dac_dma 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_dac_dma.h"

void dac_dma_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_dac_dma_t signaldacdma_init_arg0 = {0};
    static void signaldacdma_init_arg1 = {0};
    static signal_dac_dma_start_fn signaldacdma_init_arg2 = 0U;
    static signal_dac_dma_stop_fn signaldacdma_init_arg3 = 0U;
    /* ===== 调用 SignalDACDMA_Init：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDACDMA_Init(&signaldacdma_init_arg0, &signaldacdma_init_arg1, signaldacdma_init_arg2, signaldacdma_init_arg3);

    static signal_dac_dma_t signaldacdma_start_arg0 = {0};
    static uint16_t signaldacdma_start_arg1[16] = {0};
    static size_t signaldacdma_start_arg2 = 0U;
    static bool signaldacdma_start_arg3 = false;
    /* ===== 调用 SignalDACDMA_Start：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDACDMA_Start(&signaldacdma_start_arg0, signaldacdma_start_arg1, signaldacdma_start_arg2, signaldacdma_start_arg3);

    /* ===== 调用 SignalDACDMA_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDACDMA_GetModuleStatus();

    static signal_dac_dma_t signaldacdma_stop_arg0 = {0};
    /* Stop 只在需要主动取消时使用，默认流程不执行它。 */
#if 0
    (void)SignalDACDMA_Stop(&signaldacdma_stop_arg0);
#endif

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

