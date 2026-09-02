/* 工程：30_basic_measurement。教学流程：采集 → 电压换算 → 一次统计测量。 */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "ti_msp_dl_config.h"
#include "arm_math.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"

/* ADC DMA 写入的原始码；AcquireADCFrame() 返回 true 前不允许读取。 */
static uint16_t adc_samples[SIGNAL_SAMPLE_COUNT];
/* 同步双 ADC 驱动的第二路接收数组，本工程只测量 adc_samples。 */
static uint16_t adc_unused_samples[SIGNAL_SAMPLE_COUNT];
/* 物理电压样本，float/V；ConvertADCToVoltage() 写入，所有测量函数读取。 */
static float voltage_samples[SIGNAL_SAMPLE_COUNT];
/* 去 mean_v 后的交流样本，float/V；仅用于 AC RMS。 */
static float centered_samples[SIGNAL_SAMPLE_COUNT];
/* 结果各自只有一种物理含义：V 或 bool；MeasureBasicParameters() 同帧更新。 */
static float mean_v;
static float minimum_v;
static float maximum_v;
static float vpp_v;
static float rms_v;
static float ac_rms_v;
static float population_stddev_v;
static bool clipping;

static const signal_dual_adc_config_t s_adc_config = {
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
};

/* ============================================================
 * 函数：AcquireADCFrame
 * [功能] 取得一帧 DMA ADC 原始码。
 * [输入] SignalDualADC 已初始化的硬件和 s_adc_config。
 * [输出] adc_samples[]（uint16_t code）。
 * [返回值] true 为完整帧，false 为启动失败。
 * [复用] 需要 signal_dual_adc_mspm0g3507 和已验证的同步 ADC/DMA SysConfig。
 * ============================================================ */
static bool AcquireADCFrame(void)
{
    if (SignalDualADC_Start(adc_samples, adc_unused_samples,
            SIGNAL_SAMPLE_COUNT) != SIGNAL_RESULT_OK) {
        return false;
    }
    while (!SignalDualADC_IsFinished()) {
        __WFI();
    }
    return true;
}

/* ============================================================
 * [COPY START: BASIC_CONVERT]
 * 函数：ConvertADCToVoltage
 * [功能] 将 adc_samples[] 的 uint16_t ADC code 换算为 voltage_samples[]。
 * [输入] adc_samples[]；每个点范围通常为 0..4095。
 * [输出] voltage_samples[]，float，单位 V。
 * [内部步骤] voltage = code × SIGNAL_ADC_VREF_V / 4095。
 * [为什么] min/max/Vpp/RMS 等最终输出需要实际电压量纲，不能直接输出 ADC code。
 * [复用] 需要 signal_config.h 的 VREF 定义；若工程有校准，应在此函数增加原
 * 工程的校准步骤，不能静默沿用该教学默认值。
 * ============================================================ */
static void ConvertADCToVoltage(void)
{
    uint32_t index;

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        voltage_samples[index] = (float)adc_samples[index] *
            SIGNAL_ADC_VREF_V / 4095.0f;
    }
}
/* [COPY END: BASIC_CONVERT] */

/* ============================================================
 * 函数：CalculatePopulationStdDev
 * [功能] 使用 Welford 单遍累计求总体标准差，避免额外 N 点工作数组。
 * [输入] voltage_samples[]，float/V。
 * [输出] 返回总体标准差，float/V。
 * [为什么] 公式除以 N（不是 N-1），因为当前帧是完整统计总体。
 * [复用] 被 MeasureBasicParameters() 调用；不应在 main() 中展开。
 * ============================================================ */
static float CalculatePopulationStdDev(void)
{
    uint32_t index;
    float running_mean = voltage_samples[0U];
    float m2 = 0.0f;

    for (index = 1U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        const float delta = voltage_samples[index] - running_mean;
        running_mean += delta / (float)(index + 1U);
        m2 += delta * (voltage_samples[index] - running_mean);
    }
    return sqrtf(m2 / (float)SIGNAL_SAMPLE_COUNT);
}

/* ============================================================
 * [COPY START: BASIC_MEASUREMENT]
 * 函数：MeasureBasicParameters
 * [功能] 一次输出 mean/DC、min、max、Vpp、RMS、AC RMS、总体标准差和 clipping。
 * [输入] voltage_samples[]，float/V，由 ConvertADCToVoltage() 写入。
 * [输出] mean_v、minimum_v、maximum_v、vpp_v、rms_v、ac_rms_v、
 * population_stddev_v（均 V）及 clipping（bool）。
 * [内部步骤] CMSIS 求 mean/min/max/RMS → 去 mean 得 centered_samples →
 * AC RMS → Welford 标准差 → 0.02/3.28 V 的原有 clipping 门限。
 * [为什么合并] 这些量都读取同一帧电压；合并能使 main() 清楚且避免重复遍历。
 * [复用] 复制本函数还需 ConvertADCToVoltage()、CalculatePopulationStdDev()、
 * centered_samples[]、CMSIS-DSP。clipping 阈值必须按新硬件满量程复核。
 * ============================================================ */
static void MeasureBasicParameters(void)
{
    uint32_t index;
    uint32_t ignored_index;

    arm_mean_f32(voltage_samples, SIGNAL_SAMPLE_COUNT, &mean_v);
    arm_min_f32(voltage_samples, SIGNAL_SAMPLE_COUNT, &minimum_v,
        &ignored_index);
    arm_max_f32(voltage_samples, SIGNAL_SAMPLE_COUNT, &maximum_v,
        &ignored_index);
    vpp_v = maximum_v - minimum_v;
    arm_rms_f32(voltage_samples, SIGNAL_SAMPLE_COUNT, &rms_v);
    arm_offset_f32(voltage_samples, -mean_v, centered_samples,
        SIGNAL_SAMPLE_COUNT);
    arm_rms_f32(centered_samples, SIGNAL_SAMPLE_COUNT, &ac_rms_v);
    population_stddev_v = CalculatePopulationStdDev();

    clipping = false;
    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        if (voltage_samples[index] <= 0.02f ||
            voltage_samples[index] >= 3.28f) {
            clipping = true;
            break;
        }
    }
}
/* [COPY END: BASIC_MEASUREMENT] */

int main(void)
{
    SYSCFG_DL_init();
    if (SignalDualADC_Init(&s_adc_config) != SIGNAL_RESULT_OK) {
        while (true) {
        }
    }

    while (true) {
        if (!AcquireADCFrame()) {
            continue;
        }
        ConvertADCToVoltage();
        MeasureBasicParameters();
        /* 此处可读取全部最终结果并交给 TFT/UART。 */
    }
}
