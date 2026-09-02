#include "ti_msp_dl_config.h"
#include "signal_wave_output_mspm0g3507.h"

static uint16_t g_wave_table[256];
static uint16_t g_output_buffer[512];

/* 最小闭环：SysConfig 初始化完成后，初始化一次模块，再用频率/Vpp/偏置输出正弦波。 */
void wave_output_MinimalExample(void)
{
    const signal_wave_output_config_t config = {
        g_wave_table, 256U, g_output_buffer, 512U,
        {100000U, CPUCLK_FREQ, 65536U}, 12U, 3.3f
    };
    /* 该配置把静态波表、DMA 缓冲、DAC 更新率、位数和参考电压交给模块。 */
    if (SignalWaveOutput_Init(&config) != SIGNAL_RESULT_OK) return;
    /* 三个参数依次为 1000 Hz、1.0 Vpp、1.65 V 偏置。 */
    (void)SignalWaveOutput_SineWithOffset(1000.0f, 1.0f, 1.65f);
}
