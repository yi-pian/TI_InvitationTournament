#include "ti_msp_dl_config.h"
#include "signal_wave_output_mspm0g3507.h"

static uint16_t g_wave_table[256];
static uint16_t g_output_buffer[512];
static signal_wave_output_result_t g_result;

/* 全功能示例：展示初始化、四种输出、实际参数查询和停止接口。 */
void wave_output_FullExample(void)
{
    const signal_wave_output_config_t config = {
        g_wave_table, 256U, g_output_buffer, 512U,
        {100000U, CPUCLK_FREQ, 65536U}, 12U, 3.3f
    };
    /* 初始化只做一次；切换波形时不需要重新初始化。 */
    if (SignalWaveOutput_Init(&config) != SIGNAL_RESULT_OK) return;
    if (SignalWaveOutput_SineWithOffset(1000.0f, 1.0f, 1.65f) !=
        SIGNAL_RESULT_OK) return;
    /* 读取整数周期约束后的实际频率和本次 DMA 点数。 */
    (void)SignalWaveOutput_GetLastResult(&g_result);
    /* 下列三个接口的参数顺序同样是：频率 Hz、峰峰值 Vpp、偏置 V。 */
    (void)SignalWaveOutput_SquareWithDuty(1000.0f, 1.0f, 1.65f, 0.5f);
    (void)SignalWaveOutput_TriangleWithOffset(1000.0f, 1.0f, 1.65f);
    (void)SignalWaveOutput_SawtoothWithSymmetry(1000.0f, 1.0f, 1.65f, 1.0f);
    /* 正常连续输出时无需 Stop；这里只用于展示主动停止接口。 */
    SignalWaveOutput_Stop();
    (void)SignalWaveOutput_GetModuleStatus();
}
