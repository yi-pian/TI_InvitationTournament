/*
 * 工程：11_zero_cross_frequency
 *
 * 教学数据链：ADC DMA → 电压换算 → 去除 DC → 上升过零 → 线性插值
 *            → 多周期平均 → frequency_hz。
 *
 * 本文件刻意把所有教学函数保留在 main.c：比赛时复制的是完整函数，
 * 不需要新增 app/feature/core 等额外模块层。
 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "arm_math.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_zero_cross.h"
#include "signal_zero_cross_interpolation.h"

#define ZERO_EVENT_CAPACITY (128U)

/*
 * ADC 一帧原始采样数据。
 *
 * 写入方：SignalDualADC_Start() 启动的 ADC0 DMA。
 * 数据格式：uint16_t ADC code，不是物理电压；有效时刻为 AcquireADCFrame()
 * 返回 true 后。读取方：PrepareSignal()。
 */
static uint16_t adc_samples[SIGNAL_SAMPLE_COUNT];

/*
 * 第二路 DMA 的必要接收缓冲。本工程只分析第一路，仍须提供此数组以满足
 * 已验证的同步双 ADC 驱动接口；不作为测频输入。
 */
static uint16_t adc_unused_samples[SIGNAL_SAMPLE_COUNT];

/* ADC code 换算后的物理电压，float，单位 V；由 PrepareSignal() 写入。 */
static float voltage_samples[SIGNAL_SAMPLE_COUNT];

/* 去除本帧 mean_v 后的交流电压，float，单位 V；过零测量只读取此数组。 */
static float centered_samples[SIGNAL_SAMPLE_COUNT];

/* 整数采样点位置的过零事件和线性插值得到的小数采样位置。 */
static signal_zero_cross_event_t zero_events[ZERO_EVENT_CAPACITY];
static float crossing_positions[ZERO_EVENT_CAPACITY];

/* 当前硬件实际采样率，Hz；每次采集后更新。 */
static float sample_rate_hz;

/* 当前帧平均/DC 电压，V；只表示 DC，绝不临时存放频率。 */
static float mean_v;

/* 当前帧最终测量频率，Hz；仅在 MeasureFrequencyZeroCross() 成功后有效。 */
static float frequency_hz;

/* ADC/DMA 初始化参数；来自本工程既有 SysConfig 时钟配置。 */
static const signal_dual_adc_config_t s_adc_config = {
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
};

/* ============================================================
 * 函数：AcquireADCFrame
 *
 * [功能]
 * 启动同步双 ADC DMA，并等待第一路 adc_samples[] 的一帧数据完整写入。
 *
 * [输入]
 * 无显式参数。使用 s_adc_config 已初始化的 SignalDualADC 驱动。
 *
 * [输出]
 * adc_samples[]：uint16_t ADC 原始码；仅本函数返回 true 后可读取。
 * sample_rate_hz：float，Hz；定时器整数分频后的实际采样率。
 *
 * [内部主要步骤]
 * 1. SignalDualADC_Start() 启动两路 DMA；
 * 2. 以 __WFI() 等待 DMA 完成，避免忙等；
 * 3. 读取驱动报告的实际采样率。
 *
 * [返回值]
 * true：得到一帧同步采样；false：驱动拒绝本次启动。
 *
 * [如何复用]
 * 需要 signal_dual_adc_mspm0g3507.c/.h、对应双 ADC/DMA/Timer SysConfig，
 * 以及 adc_samples、adc_unused_samples、sample_rate_hz 三个全局变量。
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

    sample_rate_hz = (float)SignalDualADC_GetConfiguredRate();
    return true;
}

/* ============================================================
 * [COPY START: ZERO_CROSS_PREPARE]
 * 函数：PrepareSignal
 *
 * [功能]
 * 将 ADC 原始码转换为电压，计算本帧 mean_v，并生成去除 DC 的
 * centered_samples[]，作为固定 0 V 阈值过零检测的输入。
 *
 * [输入]
 * adc_samples[]：uint16_t ADC code，由 AcquireADCFrame() 写入。
 * SIGNAL_SAMPLE_COUNT：本帧点数。
 *
 * [输出]
 * voltage_samples[]：float，V。
 * mean_v：float，V，本帧平均/DC 电压。
 * centered_samples[]：float，V，去 DC 后的交流量。
 *
 * [内部流程]
 * ADC code → voltage_samples → arm_mean_f32(mean_v)
 *          → arm_offset_f32(-mean_v) → centered_samples。
 *
 * [为什么需要去 DC]
 * ADC 输入常带有 1.65 V 等偏置。直接以 0 V 寻找过零会没有事件或得到错误
 * 事件；去除本帧 DC 后，0 V 正好成为信号交流分量的阈值。
 *
 * [复用]
 * 复制本函数前，需先复制 AcquireADCFrame() 或提供同格式 adc_samples[]；
 * 还需要 CMSIS-DSP 的 arm_mean_f32/arm_offset_f32 和 signal_config.h。
 * ============================================================ */
static void PrepareSignal(void)
{
    uint32_t index;

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        voltage_samples[index] = (float)adc_samples[index] *
            SIGNAL_ADC_VREF_V / 4095.0f;
    }

    arm_mean_f32(voltage_samples, SIGNAL_SAMPLE_COUNT, &mean_v);
    arm_offset_f32(voltage_samples, -mean_v, centered_samples,
        SIGNAL_SAMPLE_COUNT);
}
/* [COPY END: ZERO_CROSS_PREPARE] */

/* ============================================================
 * [COPY START: ZERO_CROSS_MEASURE]
 * 函数：MeasureFrequencyZeroCross
 *
 * [功能]
 * 在去 DC 的 centered_samples[] 中找上升过零事件，对事件位置作线性插值，
 * 再利用首尾 crossing 覆盖的多个周期计算 frequency_hz。
 *
 * [输入]
 * centered_samples[]：float，V，由 PrepareSignal() 写入。
 * SIGNAL_SAMPLE_COUNT：当前帧点数。
 * sample_rate_hz：float，Hz，由 AcquireADCFrame() 写入。
 *
 * [输出]
 * zero_events[]：整数索引的上升过零事件。
 * crossing_positions[]：小数采样位置。
 * frequency_hz：float，Hz；仅返回 true 后有效。
 *
 * [内部主要步骤]
 * 1. SignalZeroCross_Process() 使用 0 V、5 mV 迟滞寻找上升沿；
 * 2. 确认至少两个有效事件；
 * 3. SignalZeroCrossInterpolation_Process() 计算亚采样 crossing 位置；
 * 4. 用首尾 crossing 的总采样距离计算多个完整周期的平均频率。
 *
 * [为什么使用多个周期]
 * 相比只测一段相邻 crossing，首尾跨度可平均单次阈值噪声和插值误差。
 *
 * [返回值]
 * true：当前帧得到有效频率；false：事件不足、插值失败或周期跨度无效。
 *
 * [复用]
 * 需连同 PrepareSignal() 一起复制，并复制 signal_zero_cross、
 * signal_zero_cross_interpolation 两个模块及其声明的依赖。
 * ============================================================ */
static bool MeasureFrequencyZeroCross(void)
{
    const signal_zero_cross_config_t zero_config = {
        0.0f, 0.005f, SIGNAL_ZERO_CROSS_RISING
    };
    signal_zero_cross_result_t zero_result;
    signal_zero_cross_interpolation_result_t interpolation_result;
    float sample_distance;

    if (SignalZeroCross_Process(centered_samples, SIGNAL_SAMPLE_COUNT,
            &zero_config, zero_events, ZERO_EVENT_CAPACITY,
            &zero_result) != SIGNAL_ALGORITHM_OK ||
        zero_result.rising_count < 2U) {
        return false;
    }

    if (SignalZeroCrossInterpolation_Process(centered_samples,
            SIGNAL_SAMPLE_COUNT, 0.0f, zero_events,
            zero_result.event_count, crossing_positions,
            ZERO_EVENT_CAPACITY, &interpolation_result) !=
            SIGNAL_ALGORITHM_OK ||
        interpolation_result.position_count < 2U) {
        return false;
    }

    sample_distance = crossing_positions[interpolation_result.position_count - 1U] -
        crossing_positions[0U];
    if (sample_distance <= 0.0f) {
        return false;
    }

    frequency_hz = ((float)(interpolation_result.position_count - 1U) *
        sample_rate_hz) / sample_distance;
    return true;
}
/* [COPY END: ZERO_CROSS_MEASURE] */

int main(void)
{
    SYSCFG_DL_init();
    if (SignalDualADC_Init(&s_adc_config) != SIGNAL_RESULT_OK) {
        while (true) {
        }
    }

    /* main() 只表达教学流程：采集 → 准备 → 测频。 */
    while (true) {
        if (!AcquireADCFrame()) {
            continue;
        }

        PrepareSignal();

        if (!MeasureFrequencyZeroCross()) {
            continue;
        }

        /* frequency_hz 此时为本帧最终结果，可接 TFT/UART/控制逻辑。 */
    }
}
