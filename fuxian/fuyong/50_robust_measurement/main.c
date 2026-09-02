/* 工程：50_robust_measurement。教学流程：采集 → 电压 → Hampel → MAD/Vpp/RMS。 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_median_filter.h"
#include "signal_hampel.h"
#include "signal_mad.h"
#include "signal_robust_peak_to_peak.h"
#include "signal_robust_rms.h"

/* ADC DMA 原始码；AcquireADCFrame() 成功后有效。 */
static uint16_t adc_samples[SIGNAL_SAMPLE_COUNT];
static uint16_t adc_unused_samples[SIGNAL_SAMPLE_COUNT];
/* 原始物理电压和选定滤波链后的电压，float/V。 */
static float voltage_samples[SIGNAL_SAMPLE_COUNT];
static float filtered_samples[SIGNAL_SAMPLE_COUNT];
/* 各原子鲁棒算法共享的临时工作区；生命周期不重叠，避免多套 N 点数组。 */
static float workspace[SIGNAL_SAMPLE_COUNT];
/* 最终结果：V、V、V 和离群点个数；AnalyzeRobustStatistics() 同帧写入。 */
static float robust_vpp_v;
static float robust_rms_v;
static float mad_v;
static uint32_t outlier_count;

typedef enum {
    ROBUST_FILTER_RAW = 0,
    ROBUST_FILTER_MEDIAN,
    ROBUST_FILTER_HAMPEL
} robust_filter_mode_t;

static const signal_dual_adc_config_t s_adc_config = {
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
};
static const signal_hampel_config_t s_hampel_config = {5U, 3.0f, 0.001f};
static const signal_robust_peak_to_peak_config_t s_vpp_config = {0.05f, 0.95f};
static const signal_robust_rms_config_t s_rms_config = {0.05f, 0.95f, 1U};
/* 保持原工程最终被 Hampel 覆盖的行为，同时明确三种教学链可选。 */
static const robust_filter_mode_t s_filter_mode = ROBUST_FILTER_HAMPEL;

/* ============================================================
 * 函数：AcquireADCFrame
 * [功能] 获得一帧 adc_samples[] 的 uint16_t ADC code。
 * [输出] true 后 adc_samples[] 可读取；false 表示 DMA 启动失败。
 * [复用] 需要 signal_dual_adc_mspm0g3507 与同一 ADC/DMA SysConfig。
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
 * [COPY START: ROBUST_CONVERT]
 * 函数：ConvertADCToVoltage
 * [功能] ADC code → voltage_samples[]，float/V。
 * [输入] adc_samples[]，uint16_t code；[输出] voltage_samples[]，float/V。
 * [为什么] Hampel、MAD 和鲁棒 RMS/Vpp 的配置与结果都以电压量纲解释。
 * [复用] 需 SIGNAL_ADC_VREF_V；有前端增益/校准时在此保留原校准定义。
 * ============================================================ */
static void ConvertADCToVoltage(void)
{
    uint32_t index;

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        voltage_samples[index] = (float)adc_samples[index] *
            SIGNAL_ADC_VREF_V / 4095.0f;
    }
}
/* [COPY END: ROBUST_CONVERT] */

/* ============================================================
 * [COPY START: MEDIAN_FILTER]
 * 函数：ApplyMedianFilter
 * [功能] 对原始电压应用 5 点中值滤波，压制单点尖峰。
 * [输入] voltage_samples[]（V）；[输出] filtered_samples[]（V）。
 * [注意] 中值滤波会改变窄脉冲；本教学工程保留该独立 COPY 函数供需要时选用。
 * [返回值] true 为算法成功。
 * [复用] 需要 signal_median_filter 与 workspace[]。
 * ============================================================ */
static bool ApplyMedianFilter(void)
{
    return SignalMedianFilter_Process(voltage_samples, filtered_samples,
        SIGNAL_SAMPLE_COUNT, 5U, workspace, SIGNAL_SAMPLE_COUNT) ==
        SIGNAL_ALGORITHM_OK;
}
/* [COPY END: MEDIAN_FILTER] */

/* ============================================================
 * [COPY START: HAMPEL_FILTER]
 * 函数：ApplyHampelFilter
 * [功能] 以局部中值/MAD 判定离群点并替换，输出 filtered_samples[] 与 outlier_count。
 * [输入] voltage_samples[]（V）、s_hampel_config。
 * [输出] filtered_samples[]（V）、outlier_count（无单位）。
 * [为什么] 相比普通均值滤波，Hampel 对少量强尖峰更稳健且保留正常样本。
 * [返回值] true 为滤波有效。
 * [复用] 需要 signal_hampel、workspace[]；若改窗口/阈值要同步说明物理影响。
 * ============================================================ */
static bool ApplyHampelFilter(void)
{
    signal_hampel_result_t hampel_result;

    if (SignalHampel_Process(voltage_samples, filtered_samples,
            SIGNAL_SAMPLE_COUNT, &s_hampel_config, workspace,
            SIGNAL_SAMPLE_COUNT, &hampel_result) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    outlier_count = hampel_result.replaced_count;
    return true;
}
/* [COPY END: HAMPEL_FILTER] */

/* ============================================================
 * 函数：ApplySelectedFilter
 * [功能] 根据 s_filter_mode 选择 RAW、Median 或 Hampel 链，并确保
 * filtered_samples[] 总是本帧鲁棒统计的唯一输入。
 * [输入] voltage_samples[]（V）和 s_filter_mode。
 * [输出] filtered_samples[]（V）；Hampel 模式同时更新 outlier_count。
 * [返回值] true：选定链成功；false：对应滤波算法失败。
 * [为什么] 原工程顺序执行中值与 Hampel，后者覆盖前者；显式选择可避免让用户
 * 误以为两条滤波链串联生效，同时保留原来的 Hampel 最终行为。
 * ============================================================ */
static bool ApplySelectedFilter(void)
{
    uint32_t index;

    switch (s_filter_mode) {
    case ROBUST_FILTER_RAW:
        for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
            filtered_samples[index] = voltage_samples[index];
        }
        outlier_count = 0U;
        return true;
    case ROBUST_FILTER_MEDIAN:
        outlier_count = 0U;
        return ApplyMedianFilter();
    case ROBUST_FILTER_HAMPEL:
    default:
        return ApplyHampelFilter();
    }
}

/* ============================================================
 * [COPY START: ROBUST_STATISTICS]
 * 函数：AnalyzeRobustStatistics
 * [功能] 对已选的 filtered_samples[] 一次完成 MAD、百分位鲁棒 Vpp 和鲁棒 RMS。
 * [输入] filtered_samples[]（float/V）。
 * [输出] mad_v、robust_vpp_v、robust_rms_v（均 float/V）。
 * [内部步骤] SignalMAD_Process → SignalRobustPeakToPeak_Process →
 * SignalRobustRMS_Process；三者依次复用同一个 workspace[]。
 * [为什么] 每项都针对同一已滤波帧，统一封装可避免 main() 堆叠三个算法调用。
 * [返回值] true 为全部指标有效。
 * [复用] 需要 MAD、robust peak-to-peak、robust RMS 三个模块和 workspace[]。
 * ============================================================ */
static bool AnalyzeRobustStatistics(void)
{
    signal_mad_result_t mad_result;
    signal_robust_peak_to_peak_result_t vpp_result;
    signal_robust_rms_result_t rms_result;

    if (SignalMAD_Process(filtered_samples, SIGNAL_SAMPLE_COUNT, workspace,
            SIGNAL_SAMPLE_COUNT, &mad_result) != SIGNAL_ALGORITHM_OK ||
        SignalRobustPeakToPeak_Process(filtered_samples, SIGNAL_SAMPLE_COUNT,
            &s_vpp_config, workspace, SIGNAL_SAMPLE_COUNT,
            &vpp_result) != SIGNAL_ALGORITHM_OK ||
        SignalRobustRMS_Process(filtered_samples, SIGNAL_SAMPLE_COUNT,
            &s_rms_config, workspace, SIGNAL_SAMPLE_COUNT,
            &rms_result) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    mad_v = mad_result.mad_value;
    robust_vpp_v = vpp_result.robust_vpp_v;
    robust_rms_v = rms_result.robust_rms_v;
    return true;
}
/* [COPY END: ROBUST_STATISTICS] */

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
        if (!ApplySelectedFilter()) {
            continue;
        }
        (void)AnalyzeRobustStatistics();
    }
}
