/* adc_dma 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_adc_dma.h"

void adc_dma_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_adc_dma_config_t signaladc_init_arg0 = {0};
    /* ===== 调用 SignalADC_Init：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADC_Init(&signaladc_init_arg0);

    static uint32_t signaladc_setsamplerate_arg0 = 0U;
    /* ===== 调用 SignalADC_SetSampleRate：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADC_SetSampleRate(signaladc_setsamplerate_arg0);

    static uint16_t signaladc_start_arg0[16] = {0};
    static uint16_t signaladc_start_arg1 = 0U;
    /* ===== 调用 SignalADC_Start：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADC_Start(signaladc_start_arg0, signaladc_start_arg1);

    /* ===== 调用 SignalADC_IsFinished：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADC_IsFinished();

    /* ===== 调用 SignalADC_GetBuffer：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADC_GetBuffer();

    /* ===== 调用 SignalADC_GetConfiguredTriggerRate：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADC_GetConfiguredTriggerRate();

    /* ===== 调用 SignalADC_GetModuleMaturity：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADC_GetModuleMaturity();

    /* ===== 调用 SignalADC_GetSampleCount：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADC_GetSampleCount();

    /* ===== 调用 SignalADC_GetStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalADC_GetStatus();

    /* Stop 只在需要主动取消时使用，默认流程不执行它。 */
#if 0
    (void)SignalADC_Stop();
#endif

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

