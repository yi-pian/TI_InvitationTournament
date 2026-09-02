/* 工程：90_dds_usage。教学流程：初始化 DDS/DAC DMA → 设置频率 → 获取结果。 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_wave_output_mspm0g3507.h"

/* DDS 正弦查找表与 DAC DMA 输出表；仅 SignalWaveOutput_Init() 和模块内部写入。 */
static uint16_t wave_table[256U];
static uint16_t dac_output[512U];
/* 用户请求的正弦频率，float/Hz；只表示频率，不能临时用于相位或幅值。 */
static float frequency_hz = 1000.0f;
/* 模块返回的实际 DDS/DMA 参数，SetDDSFrequency() 后可读取。 */
static signal_wave_output_result_t dds_result;

static const signal_dac_dma_mspm0_config_t s_dac_config = {
    SIGNAL_DAC_UPDATE_RATE_HZ, CPUCLK_FREQ, 65536U
};

/* ============================================================
 * [COPY START: DDS_INIT]
 * 函数：InitDDSOutput
 * [功能] 连接波表、DAC DMA 缓冲和既有 SysConfig 定时器，初始化 DDS 输出模块。
 * [输入] wave_table[]、dac_output[]、s_dac_config、12 bit 与 VREF 配置。
 * [输出] true 后模块可调用 SetDDSFrequency() 驱动 DAC/DMA。
 * [为什么] DDS 的 phase accumulator、波表索引和 DMA 更新率由模块管理，main()
 * 不应手工复制其内部实现。
 * [复用] 需要 signal_wave_output_mspm0g3507 及其声明的 DAC DMA 模块和对应 SysConfig。
 * ============================================================ */
static bool InitDDSOutput(void)
{
    const signal_wave_output_config_t config = {
        wave_table, 256U, dac_output, 512U, s_dac_config, 12U,
        SIGNAL_ADC_VREF_V
    };

    return SignalWaveOutput_Init(&config) == SIGNAL_RESULT_OK;
}
/* [COPY END: DDS_INIT] */

/* ============================================================
 * [COPY START: DDS_SET_FREQUENCY]
 * 函数：SetDDSFrequency
 * [功能] 以 frequency_hz 请求模块重新生成带 1.65 V 偏置、1.0 V 幅度的正弦输出。
 * [输入] frequency_hz：float/Hz。
 * [输出] DAC DMA 正弦波和 dds_result；实际频率由波表长度和更新率量化。
 * [内部关系] DAC update rate + 波表长度 + DDS phase accumulator 决定输出频率；
 * 本函数不直接计算 phase accumulator，沿用已验证模块。
 * [返回值] true：成功更新；false：模块拒绝参数。
 * [复用] 需先 InitDDSOutput()；幅值/偏置改变时必须确认 DAC 满量程和模拟前端。
 * ============================================================ */
static bool SetDDSFrequency(void)
{
    if (SignalWaveOutput_SineWithOffset(frequency_hz, 1.0f,
            1.65f) != SIGNAL_RESULT_OK) {
        return false;
    }
    (void)SignalWaveOutput_GetLastResult(&dds_result);
    return true;
}
/* [COPY END: DDS_SET_FREQUENCY] */

/* ============================================================
 * [COPY START: DDS_KEY_ADJUST]
 * 函数：HandleDDSFrequencyAdjust
 * [功能] 作为键盘/串口参数输入后的唯一 DDS 更新入口。
 * [输入] requested_frequency_hz：float/Hz；[输出] frequency_hz 和 DAC 波形。
 * [为什么] 让 UI 只修改一个频率变量，实际输出始终通过 SetDDSFrequency() 更新。
 * [复用] 本工程不绑定 70_keypad_usage；调用方须自行完成数值输入、范围检查。
 * ============================================================ */
static bool HandleDDSFrequencyAdjust(float requested_frequency_hz)
{
    frequency_hz = requested_frequency_hz;
    return SetDDSFrequency();
}
/* [COPY END: DDS_KEY_ADJUST] */

int main(void)
{
    SYSCFG_DL_init();
    if (!InitDDSOutput() || !SetDDSFrequency()) {
        while (true) {
        }
    }

    while (true) {
        /* 原工程每轮刷新当前请求；键盘/串口可先改 frequency_hz 再进入该函数。 */
        (void)HandleDDSFrequencyAdjust(frequency_hz);
        (void)dds_result;
        __WFI();
    }
}
