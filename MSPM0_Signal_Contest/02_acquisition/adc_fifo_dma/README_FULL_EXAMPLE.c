/* adc_fifo_dma 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_adc_fifo_dma.h"

void adc_fifo_dma_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_adc_fifo_dma_config_t signaladcfifodma_init_arg0 = {0};
    /* ===== 调用 SignalADCFIFODMA_Init：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADCFIFODMA_Init(&signaladcfifodma_init_arg0);

    static uint16_t signaladcfifodma_start_arg0[16] = {0};
    static uint16_t signaladcfifodma_start_arg1 = 0U;
    /* ===== 调用 SignalADCFIFODMA_Start：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADCFIFODMA_Start(signaladcfifodma_start_arg0, signaladcfifodma_start_arg1);

    /* ===== 调用 SignalADCFIFODMA_IsFinished：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADCFIFODMA_IsFinished();

    /* ===== 调用 SignalADCFIFODMA_GetBuffer：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADCFIFODMA_GetBuffer();

    /* ===== 调用 SignalADCFIFODMA_GetModuleMaturity：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADCFIFODMA_GetModuleMaturity();

    /* ===== 调用 SignalADCFIFODMA_GetNominalSampleRateHz：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADCFIFODMA_GetNominalSampleRateHz();

    /* ===== 调用 SignalADCFIFODMA_GetSampleCount：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADCFIFODMA_GetSampleCount();

    /* ===== 调用 SignalADCFIFODMA_GetStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADCFIFODMA_GetStatus();

    /* Stop 只在需要主动取消时使用，默认流程不执行它。 */
#if 0
    (void)SignalADCFIFODMA_Stop();
#endif

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

