/*
 * 工程：20_fft_analysis
 *
 * 本工程的唯一 FFT 数据链：ADC DMA → PrepareSignal() → RunFFTCommon()
 * → fft_magnitude[] → 频率/插值/谐波/THD/SNR/SFDR/频谱显示。
 *
 * 注意：每帧仅 RunFFTCommon() 调用一次 arm_cfft_q15。后续所有分析函数只读
 * fft_magnitude[]，绝不能各自重新执行 FFT。
 */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "ti_msp_dl_config.h"
#include "arm_const_structs.h"
#include "arm_math.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_window.h"
#include "signal_window_gain_correction.h"
#include "signal_fft_parabolic_interpolation.h"
#include "signal_harmonic.h"
#include "signal_thd.h"
#include "signal_snr.h"
#include "signal_sfdr.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_mspm0g3507.h"

#define SPECTRUM_X (8)
#define SPECTRUM_Y (48)
#define SPECTRUM_W (304)
#define SPECTRUM_H (160)

/* ADC0 DMA 写入的 uint16_t code；AcquireADCFrame() 成功后才可读取。 */
static uint16_t adc_samples[SIGNAL_SAMPLE_COUNT];
/* 同步双 ADC 驱动需要的第二路 DMA 接收缓冲，本工程不参与频谱计算。 */
static uint16_t adc_unused_samples[SIGNAL_SAMPLE_COUNT];
/* ADC code 换算的物理电压，float，V；PrepareSignal() 写入。 */
static float voltage_samples[SIGNAL_SAMPLE_COUNT];
/* 去 DC 后再加窗的中间交流电压，float，V；PrepareSignal() 写入。 */
static float centered_samples[SIGNAL_SAMPLE_COUNT];
/* 单边 FFT 幅度，float，V；RunFFTCommon() 是唯一写入方，之后全部分析只读。 */
static float fft_magnitude[(SIGNAL_SAMPLE_COUNT / 2U) + 1U];
/* Q15 复数 FFT 工作区和 CMSIS 幅度临时数据；仅 RunQ15FFT() 使用。 */
static q15_t fft_q15[2U * SIGNAL_SAMPLE_COUNT];
static q15_t fft_magnitude_q15[SIGNAL_SAMPLE_COUNT];

/* 实际采样率 Hz、直流均值 V、最终频率 Hz 与峰值幅度 V。 */
static float sample_rate_hz;
static float mean_v;
static float frequency_hz;
static float peak_value;
static float interpolated_bin;
static float thd_percent;
/* 多 bin RSS 换算为正弦峰值幅度：coherent_gain / sqrt(power_gain)。 */
static float fft_harmonic_amplitude_correction = 1.0f;
static float snr_db;
static float sfdr_db;
static uint32_t peak_bin;

static signal_fft_parabolic_result_t interpolation_result;
static signal_harmonic_result_t harmonics;
static signal_thd_result_t thd_result;
static signal_snr_result_t snr_result;
static signal_sfdr_result_t sfdr_result;
static tft_st7789_t tft;

static const signal_dual_adc_config_t s_adc_config = {
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
};

/* ============================================================
 * 函数：AcquireADCFrame
 *
 * [功能] 启动 ADC DMA 并取得一帧 adc_samples[]。
 * [输入] 已由 SysConfig 和 SignalDualADC_Init() 建立的双 ADC/DMA/Timer。
 * [输出] adc_samples[]（uint16_t code）和 sample_rate_hz（Hz）。
 * [主要步骤] Start → WFI 等待 DMA 完成 → 读取实际 Timer 触发频率。
 * [返回值] true 为帧有效，false 为本次 DMA 启动失败。
 * [复用] 复制本函数需要 signal_dual_adc_mspm0g3507 和相同 SysConfig。
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
 * [COPY START: FFT_PREPARE]
 * 函数：PrepareSignal
 *
 * [功能] 将 uint16_t ADC code 转换为 V，计算 mean_v，并得到 centered_samples。
 * [输入] adc_samples[]、SIGNAL_SAMPLE_COUNT。
 * [输出] voltage_samples[]（V）、mean_v（V）、centered_samples[]（V）。
 * [内部步骤] ADC code → voltage → arm_mean_f32 → offset(-mean_v)。
 * [为什么] 直流偏置会占满 FFT 的 0 Hz bin，并使后续频谱动态范围变差。
 * [复用] 测频、谐波与时域交流分析均需本函数；依赖 CMSIS-DSP 与 signal_config。
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
/* [COPY END: FFT_PREPARE] */

/* ============================================================
 * 函数：RunQ15FFT
 *
 * [功能] 将已加窗的 float 电压帧归一化成 Q15，调用 CMSIS 复数 FFT，并恢复
 * 单边幅度的 float 量纲。
 * [输入] input：float，V，长度 SIGNAL_SAMPLE_COUNT。
 * [输出] fft_magnitude[]：float，V。
 * [主要步骤] 查 max_abs 防溢出 → float 转 Q15 复数 → arm_cfft_q15 →
 * arm_cmplx_mag_q15 → 恢复量纲。
 * [返回值] true：FFT 或全零输入已得到有效幅度数组。
 * [复用] 仅供 RunFFTCommon() 调用；复制 FFT_COMMON 时必须一并复制本函数、
 * fft_q15 和 fft_magnitude_q15 工作区，以及 CMSIS Q15 FFT 支持。
 * ============================================================ */
static bool RunQ15FFT(const float *input)
{
    const arm_cfft_instance_q15 *instance = &arm_cfft_sR_q15_len1024;
    uint32_t index;
    float max_abs = 0.0f;
    float restore_scale;

#if SIGNAL_SAMPLE_COUNT == 512U
    instance = &arm_cfft_sR_q15_len512;
#elif SIGNAL_SAMPLE_COUNT != 1024U
#error "20_fft_analysis currently documents the verified 512/1024 Q15 CMSIS sizes"
#endif

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        const float abs_value = fabsf(input[index]);
        if (abs_value > max_abs) {
            max_abs = abs_value;
        }
    }
    if (max_abs == 0.0f) {
        arm_fill_f32(0.0f, fft_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U);
        return true;
    }

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        fft_q15[2U * index] = (q15_t)((input[index] / max_abs) * 32767.0f);
        fft_q15[2U * index + 1U] = 0;
    }
    arm_cfft_q15(instance, fft_q15, 0U, 1U);
    arm_cmplx_mag_q15(fft_q15, fft_magnitude_q15, SIGNAL_SAMPLE_COUNT);

    /* arm_cmplx_mag_q15 输出 Q2.14，数值 1.0 对应 16384。 */
    restore_scale = max_abs * (float)SIGNAL_SAMPLE_COUNT / 16384.0f;
    for (index = 0U; index <= SIGNAL_SAMPLE_COUNT / 2U; ++index) {
        fft_magnitude[index] = (float)fft_magnitude_q15[index] * restore_scale;
    }
    return true;
}

/* ============================================================
 * [COPY START: FFT_COMMON_HELPER_LOW_RAM]
 * 函数：RunQ15FFTLowRam
 *
 * [功能] 与 RunQ15FFT() 相同：产生 fft_magnitude[]（V）；但不使用
 * fft_magnitude_q15[SIGNAL_SAMPLE_COUNT] 临时数组。
 * [内存] 只需 fft_q15[2*N]（4N 字节）和 fft_magnitude[N/2+1]（约 2N 字节），
 *        相比原版少 2N 字节。1024 点 FFT 可节省 2048 B。
 * [原理] arm_cmplx_mag_q15() 对每个复数点计算 sqrt((re^2+im^2)/2) 并输出 Q2.14；
 *        本函数按同一公式逐点计算后直接写 float 幅值，避免保存整帧 Q15 幅值。
 * [复用] 复制时保留 fft_magnitude[]、fft_q15[]，不要复制 fft_magnitude_q15[]；
 *        在 FFT_COMMON 中把 RunQ15FFT() 替换为 RunQ15FFTLowRam()。
 * ============================================================ */
static bool RunQ15FFTLowRam(const float *input)
{
    const arm_cfft_instance_q15 *instance = &arm_cfft_sR_q15_len1024;
    uint32_t index;
    float max_abs = 0.0f;
    float restore_scale;

#if SIGNAL_SAMPLE_COUNT == 512U
    instance = &arm_cfft_sR_q15_len512;
#elif SIGNAL_SAMPLE_COUNT != 1024U
#error "20_fft_analysis currently documents the verified 512/1024 Q15 CMSIS sizes"
#endif

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        const float abs_value = fabsf(input[index]);
        if (abs_value > max_abs) {
            max_abs = abs_value;
        }
    }
    if (max_abs == 0.0f) {
        arm_fill_f32(0.0f, fft_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U);
        return true;
    }

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        fft_q15[2U * index] =
            (q15_t)((input[index] / max_abs) * 32767.0f);
        fft_q15[2U * index + 1U] = 0;
    }
    arm_cfft_q15(instance, fft_q15, 0U, 1U);

    /* 与 arm_cmplx_mag_q15 一致：手算幅值也是 Q2.14。 */
    restore_scale = max_abs * (float)SIGNAL_SAMPLE_COUNT / 16384.0f;
    for (index = 0U; index <= SIGNAL_SAMPLE_COUNT / 2U; ++index) {
        q15_t real = fft_q15[2U * index];
        q15_t imag = fft_q15[2U * index + 1U];
        q31_t magnitude_q31;
        uint32_t magnitude_square =
            (((uint32_t)((q31_t)real * real) +
              (uint32_t)((q31_t)imag * imag)) >> 1U);

        if (arm_sqrt_q31((q31_t)magnitude_square,
                &magnitude_q31) != ARM_MATH_SUCCESS) {
            return false;
        }
        fft_magnitude[index] =
            (float)(magnitude_q31 >> 16U) * restore_scale;
    }
    return true;
}
/* [COPY END: FFT_COMMON_HELPER_LOW_RAM] */

/* ============================================================
 * [COPY START: FFT_COMMON]
 * 函数：RunFFTCommon
 *
 * [功能] 对 centered_samples[] 应用 Hann 窗、执行本帧唯一一次 FFT，并校正
 * 窗的 coherent gain，最终产生可被所有后续算法共用的 fft_magnitude[]。
 * [输入] centered_samples[]（float，V），由 PrepareSignal() 生成。
 * [输出] fft_magnitude[]（float，V）；本帧所有频谱分析的唯一数据来源。
 * [内部步骤] Hann → RunQ15FFT() → WindowGainCorrection。
 * [为什么] Hann 降低非整周期泄漏；coherent gain 修正避免窗函数改变幅度量级。
 * [返回值] true：可继续测频/谐波；false：窗或增益修正失败。
 * [复用] 需要 PrepareSignal()、RunQ15FFT()、signal_window 和
 * signal_window_gain_correction。后续函数不得重新 FFT。
 * ============================================================ */
static bool RunFFTCommon(void)
{
    signal_window_result_t window_result;

    if (SignalWindow_Apply(centered_samples, voltage_samples,
            SIGNAL_SAMPLE_COUNT, SIGNAL_WINDOW_HANN,
            &window_result) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    if (!RunQ15FFT(voltage_samples)) {
        return false;
    }
    if (SignalWindowGainCorrection_Apply(fft_magnitude, fft_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U, SIGNAL_SAMPLE_COUNT,
            window_result.coherent_gain) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    fft_harmonic_amplitude_correction = window_result.coherent_gain /
        sqrtf(window_result.power_gain);
    return isfinite(fft_harmonic_amplitude_correction) &&
        (fft_harmonic_amplitude_correction > 0.0f);
}
/* [COPY END: FFT_COMMON] */

/* ============================================================
 * [COPY START: FFT_COMMON_LOW_RAM_PARAMETERIZED]
 * 函数：RunFFTCommonLowRam
 *
 * [功能] 对任一通道的去直流样本加 Hann 窗，并用低 RAM Q15 FFT 产生全局
 * fft_magnitude[]。同一个工作区可先后处理 CH1、CH2，不需要两份 FFT 缓冲。
 * [输入] centered_input：去 DC 电压；fft_input：长度 N 的临时 float 工作区。
 * [输出] fft_magnitude[]：当前输入通道的单边线性幅值（V）。
 * [复用] 需要 FFT_COMMON_HELPER_LOW_RAM、signal_window、
 * signal_window_gain_correction。调用者处理完一个通道的结果后，必须先保存
 * 自己需要的数值，再用本函数处理下一个通道；fft_magnitude[] 会被覆盖。
 * ============================================================ */
static bool RunFFTCommonLowRam(const float *centered_input, float *fft_input)
{
    signal_window_result_t window_result;

    if (SignalWindow_Apply(centered_input, fft_input,
            SIGNAL_SAMPLE_COUNT, SIGNAL_WINDOW_HANN,
            &window_result) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    if (!RunQ15FFTLowRam(fft_input)) {
        return false;
    }
    if (SignalWindowGainCorrection_Apply(fft_magnitude, fft_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U, SIGNAL_SAMPLE_COUNT,
            window_result.coherent_gain) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    fft_harmonic_amplitude_correction = window_result.coherent_gain /
        sqrtf(window_result.power_gain);
    return isfinite(fft_harmonic_amplitude_correction) &&
        (fft_harmonic_amplitude_correction > 0.0f);
}
/* [COPY END: FFT_COMMON_LOW_RAM_PARAMETERIZED] */

/* ============================================================
 * [COPY START: FFT_FREQUENCY]
 * 函数：MeasureFFTFrequency
 *
 * [功能] 在 fft_magnitude[] 的非 DC 区寻找最大谱线，得到整数 bin 测频结果。
 * [输入] fft_magnitude[]、sample_rate_hz。
 * [输出] peak_bin（整数 bin）、peak_value（V）、frequency_hz（Hz）。
 * [为什么跳过 DC] DC 已在 PrepareSignal() 去除，但仍不把 bin 0 当作信号峰值。
 * [返回值] true：得到峰值；false：全部谱线为零。
 * [复用] 需要 RunFFTCommon()；依赖 CMSIS arm_max_f32。
 * ============================================================ */
static bool MeasureFFTFrequency(void)
{
    arm_max_f32(&fft_magnitude[1], SIGNAL_SAMPLE_COUNT / 2U,
        &peak_value, &peak_bin);
    peak_bin += 1U;
    if (peak_value <= 0.0f) {
        return false;
    }
    frequency_hz = (float)peak_bin * sample_rate_hz /
        (float)SIGNAL_SAMPLE_COUNT;
    return true;
}
/* [COPY END: FFT_FREQUENCY] */

/* ============================================================
 * [COPY START: FFT_PEAK_INTERPOLATION]
 * 函数：RefineFFTFrequency
 *
 * [功能] 用 peak_bin 相邻三点的抛物线插值，把整数 bin 频率精修为小数 bin。
 * [输入] fft_magnitude[]、peak_bin、sample_rate_hz。
 * [输出] interpolated_bin（bin）、frequency_hz（Hz）。
 * [返回值] true：插值成功；false：峰在边界或三点条件不满足。
 * [复用] 依赖 MeasureFFTFrequency() 与 signal_fft_parabolic_interpolation。
 * ============================================================ */
static bool RefineFFTFrequency(void)
{
    if (SignalFFTParabolicInterpolation_Process(fft_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U, peak_bin, sample_rate_hz,
            SIGNAL_SAMPLE_COUNT, &interpolation_result) !=
            SIGNAL_ALGORITHM_OK) {
        return false;
    }
    interpolated_bin = interpolation_result.fractional_bin;
    frequency_hz = interpolation_result.frequency_hz;
    return true;
}
/* [COPY END: FFT_PEAK_INTERPOLATION] */

/* ============================================================
 * [COPY START: FFT_HARMONICS_THD]
 * 函数：AnalyzeHarmonicsAndTHD
 *
 * [功能] 以精修后的 frequency_hz 为基波，提取 H1~H3，并由该结果计算 THD。
 * [输入] fft_magnitude[]、frequency_hz、sample_rate_hz。
 * [输出] harmonics、thd_percent（%）。
 * [为什么] 谐波中心采用插值后的基波频率，可减少非整数 bin 带来的偏差。
 * [返回值] true：谐波和 THD 都有效；false：任一算法返回失败。
 * [复用] 需要 RunFFTCommon() 和 RefineFFTFrequency()；依赖 harmonic/thd 模块。
 * ============================================================ */
static bool AnalyzeHarmonicsAndTHD(void)
{
    const signal_harmonic_config_t harmonic_config = {
        frequency_hz, 1U, 3U, 1U
    };

    if (SignalHarmonic_Process(fft_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U, sample_rate_hz,
            SIGNAL_SAMPLE_COUNT, &harmonic_config,
            &harmonics) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    if (SignalTHD_Process(&harmonics, &thd_result) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    thd_percent = thd_result.thd_percent;
    return true;
}
/* [COPY END: FFT_HARMONICS_THD] */

/* ============================================================
 * [COPY START: FFT_HARMONICS_THD_PARAMETERIZED]
 * 函数：AnalyzeHarmonicsAndTHDToOrder
 *
 * [功能] 从已得到的 frequency_hz 和 fft_magnitude[] 提取 H1 至指定最高阶，
 *        并计算 THD。THD 自动为 sqrt(H2^2+...+Hn^2) / H1。
 * [输入] last_order：最高谐波阶数，范围 2～SIGNAL_HARMONIC_MAX_ORDER。
 * [输出] harmonics、thd_percent（%）。Hn 的峰值幅度读取
 *        harmonics.items[n].root_sum_square。
 * [复用] H2～H5 时调用 AnalyzeHarmonicsAndTHDToOrder(5U)；依赖
 *        FFT_FREQUENCY、FFT_PEAK_INTERPOLATION、signal_harmonic、signal_thd。
 * ============================================================ */
static bool AnalyzeHarmonicsAndTHDToOrder(uint32_t last_order)
{
    const signal_harmonic_config_t harmonic_config = {
        frequency_hz, 1U, last_order, 1U
    };

    if (SignalHarmonic_Process(fft_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U, sample_rate_hz,
            SIGNAL_SAMPLE_COUNT, &harmonic_config,
            &harmonics) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    if (SignalTHD_Process(&harmonics, &thd_result) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    thd_percent = thd_result.thd_percent;
    return true;
}
/* [COPY END: FFT_HARMONICS_THD_PARAMETERIZED] */

/* ============================================================
 * [COPY START: FFT_CHANNEL_HARMONICS_MEASURE]
 * 函数：MeasureChannelHarmonics
 *
 * [功能] 对一个通道完成“FFT → 基波峰值/插值 → H1~Hn → THD”的完整数据链。
 * [输入] centered_input：去 DC 数据；fft_input：长度 N 的加窗工作区；
 *        last_order：最高谐波阶数（例如 5U）。
 * [输出] fundamental_frequency_hz：FFT 插值基波频率；
 *        harmonic_amplitude_v[0..n-1] 分别为 H1~Hn 的峰值幅度；
 *        thd_percent_out：THD 百分比。
 * [内存] fft_magnitude[] 仍是共用工作区；调用完成后结果已复制到输出数组，
 *        可以立即再调用本函数处理另一通道。
 * [复用] 双通道 H1~H5 时，对 X、Y 各调用一次，last_order 都传 5U。
 * ============================================================ */
static bool MeasureChannelHarmonics(
    const float *centered_input,
    float *fft_input,
    uint32_t last_order,
    float *fundamental_frequency_hz,
    float *harmonic_amplitude_v,
    float *thd_percent_out)
{
    uint32_t order;

    if ((last_order < 2U) || (last_order > SIGNAL_HARMONIC_MAX_ORDER)) {
        return false;
    }
    if (!RunFFTCommonLowRam(centered_input, fft_input) ||
        !MeasureFFTFrequency() || !RefineFFTFrequency() ||
        !AnalyzeHarmonicsAndTHDToOrder(last_order)) {
        return false;
    }

    *fundamental_frequency_hz = frequency_hz;
    for (order = 1U; order <= last_order; ++order) {
        harmonic_amplitude_v[order - 1U] =
            harmonics.items[order].root_sum_square *
            fft_harmonic_amplitude_correction;
    }
    *thd_percent_out = thd_percent;
    return true;
}
/* [COPY END: FFT_CHANNEL_HARMONICS_MEASURE] */

/* ============================================================
 * [COPY START: FFT_DUAL_CHANNEL_HARMONICS_MEASURE]
 * 函数：MeasureDualChannelHarmonics
 *
 * [功能] 用同一套 FFT 工作区依次测量两个通道。先完成 CH1 并保存输出，
 *        再覆盖工作区测 CH2，因此不会为双通道重复分配 fft_q15/magnitude。
 * [输入] 两路 centered_input 与各自长度 N 的 float 加窗工作区；last_order。
 * [输出] 两路频率、H1~Hn 峰值幅度数组、THD 百分比。
 * [返回] 有效位掩码：bit0=CH1 有效，bit1=CH2 有效。一路失败不影响另一路分析。
 * ============================================================ */
static uint8_t MeasureDualChannelHarmonics(
    const float *centered_x,
    float *fft_input_x,
    const float *centered_y,
    float *fft_input_y,
    uint32_t last_order,
    float *fundamental_frequency_x_hz,
    float *harmonic_amplitude_x_v,
    float *thd_x_percent,
    float *fundamental_frequency_y_hz,
    float *harmonic_amplitude_y_v,
    float *thd_y_percent)
{
    uint8_t valid_mask = 0U;

    if (MeasureChannelHarmonics(centered_x, fft_input_x, last_order,
            fundamental_frequency_x_hz, harmonic_amplitude_x_v,
            thd_x_percent)) {
        valid_mask |= 0x01U;
    }
    if (MeasureChannelHarmonics(centered_y, fft_input_y, last_order,
            fundamental_frequency_y_hz, harmonic_amplitude_y_v,
            thd_y_percent)) {
        valid_mask |= 0x02U;
    }
    return valid_mask;
}
/* [COPY END: FFT_DUAL_CHANNEL_HARMONICS_MEASURE] */

/* ============================================================
 * [COPY START: FFT_SNR_SFDR]
 * 函数：AnalyzeSNRAndSFDR
 *
 * [功能] 忽略基波周围 3 个 bin 后，用同一 fft_magnitude[] 计算 SNR 与 SFDR。
 * [输入] fft_magnitude[]、peak_bin。
 * [输出] snr_db、sfdr_db，单位 dB。
 * [返回值] true：两个指标均有效；false：任一指标计算失败。
 * [复用] 需先 MeasureFFTFrequency()；依赖 signal_snr、signal_sfdr；不做 FFT。
 * ============================================================ */
static bool AnalyzeSNRAndSFDR(void)
{
    const signal_snr_config_t snr_config = {
        peak_bin - 1U, peak_bin + 1U, 1U, SIGNAL_SAMPLE_COUNT / 2U, NULL, 0U
    };
    const signal_sfdr_config_t sfdr_config = {
        peak_bin - 1U, peak_bin + 1U, 1U, SIGNAL_SAMPLE_COUNT / 2U
    };

    if (SignalSNR_Process(fft_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U, &snr_config,
            &snr_result) != SIGNAL_ALGORITHM_OK ||
        SignalSFDR_Process(fft_magnitude,
            (SIGNAL_SAMPLE_COUNT / 2U) + 1U, &sfdr_config,
            &sfdr_result) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    snr_db = snr_result.snr_db;
    sfdr_db = sfdr_result.sfdr_db;
    return true;
}
/* [COPY END: FFT_SNR_SFDR] */

/* ============================================================
 * [COPY START: FFT_SPECTRUM_PLOT]
 * 函数：DrawFFTSpectrum
 *
 * [功能] 把已有的 fft_magnitude[] 缩放到 TFT 谱线；只显示，不参与算法。
 * [输入] fft_magnitude[]、peak_value；幅度为 float V。
 * [输出] TFT 频谱折线。横轴为 bin 映射，纵轴按本帧峰值归一化。
 * [为什么按列抽样] SIGNAL_SAMPLE_COUNT/2 通常大于屏宽，逐列取对应 bin
 * 能保留完整频带而无需申请新的显示缓冲。
 * [复用] 需 TFT ST7789 模块、已初始化的 tft，以及 MeasureFFTFrequency()。
 * 本函数绝不执行 FFT。
 * ============================================================ */
static void DrawFFTSpectrum(void)
{
    uint32_t index;

    (void)TFT_ST7789_FillRect(&tft, SPECTRUM_X, SPECTRUM_Y,
        SPECTRUM_W, SPECTRUM_H, TFT_ST7789_BLACK);
    if (peak_value <= 0.0f) {
        return;
    }
    for (index = 1U; index < (uint32_t)SPECTRUM_W; ++index) {
        const uint32_t bin0 = (index - 1U) * (SIGNAL_SAMPLE_COUNT / 2U) /
            (uint32_t)SPECTRUM_W;
        const uint32_t bin1 = index * (SIGNAL_SAMPLE_COUNT / 2U) /
            (uint32_t)SPECTRUM_W;
        const int32_t y0 = SPECTRUM_Y + SPECTRUM_H - 1 -
            (int32_t)(fft_magnitude[bin0] * (float)(SPECTRUM_H - 1) / peak_value);
        const int32_t y1 = SPECTRUM_Y + SPECTRUM_H - 1 -
            (int32_t)(fft_magnitude[bin1] * (float)(SPECTRUM_H - 1) / peak_value);
        (void)TFT_ST7789_DrawLine(&tft, SPECTRUM_X + (int32_t)index - 1,
            y0, SPECTRUM_X + (int32_t)index, y1, TFT_ST7789_CYAN);
    }
}
/* [COPY END: FFT_SPECTRUM_PLOT] */

int main(void)
{
    SYSCFG_DL_init();
    if (SignalDualADC_Init(&s_adc_config) != SIGNAL_RESULT_OK) {
        while (true) {
        }
    }
    if (SignalTFTST7789_MSPM0_Init(&tft, TFT_ST7789_ROTATION_270,
            0U, 0U) != TFT_ST7789_OK) {
        while (true) {
        }
    }

    /* main() 作为流程图：采集 → 准备 → 唯一 FFT → 分析 → 显示。 */
    while (true) {
        if (!AcquireADCFrame()) {
            continue;
        }
        PrepareSignal();
        if (!RunFFTCommon() || !MeasureFFTFrequency()) {
            continue;
        }
        (void)RefineFFTFrequency();
        (void)AnalyzeHarmonicsAndTHD();
        (void)AnalyzeSNRAndSFDR();
        DrawFFTSpectrum();
    }
}
