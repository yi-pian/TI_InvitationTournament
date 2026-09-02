/* 工程：10_timer_frequency。教学流程：初始化 Capture → 读取周期/频率/占空比。 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_timer_capture_mspm0g3507.h"

/* 硬件 Capture 得出的最终物理结果；frequency_hz/Hz、duty_cycle_percent/%。 */
static float frequency_hz;
static float duty_cycle_percent;
/* 模块原始结果，MeasureTimerFrequency() 写入，包含 valid 与 Capture 细节。 */
static signal_timer_capture_mspm0_result_t capture_result;
static const signal_timer_capture_mspm0_config_t s_capture_config = {
    CPUCLK_FREQ, SIGNAL_CAPTURE_INST_LOAD_VALUE
};

/* ============================================================
 * 函数：InitTimerFrequencyMeasurement
 * [功能] 初始化比较器事件到 TIMG Capture 的既有硬件测频模块，并启动捕获。
 * [输入] CPUCLK_FREQ 和 SysConfig 生成的 Capture load value。
 * [输出] true 后 Capture 持续记录输入边沿；false 表示初始化/启动失败。
 * [复用] 需要 signal_timer_capture_mspm0g3507 和本工程 COMP/TIMG SysConfig。
 * ============================================================ */
static bool InitTimerFrequencyMeasurement(void)
{
    return SignalTimerCapture_MSPM0_Init(&s_capture_config) == SIGNAL_RESULT_OK &&
        SignalTimerCapture_MSPM0_Start() == SIGNAL_RESULT_OK;
}

/* ============================================================
 * [COPY START: TIMER_CAPTURE]
 * 函数：MeasureTimerFrequency
 * [功能] 读取模块已经由硬件 Capture 得到的周期、频率和占空比。
 * [输入] COMP/TIMG Capture 硬件事件；不使用 adc_samples。
 * [输出] frequency_hz（Hz）、duty_cycle_percent（%）。
 * [内部原理] 模块根据 capture count 和定时器时钟得 period，再换算 frequency。
 * [返回值] true：capture_result.valid 且结果有效；false：当前尚未捕到完整周期。
 * [复用] 需要 InitTimerFrequencyMeasurement()；不能将 ADC DMA 结果冒充 Timer Capture。
 * ============================================================ */
static bool MeasureTimerFrequency(void)
{
    if (SignalTimerCapture_MSPM0_GetResult(&capture_result) != SIGNAL_RESULT_OK ||
        !capture_result.valid) {
        return false;
    }
    frequency_hz = capture_result.frequency_hz;
    duty_cycle_percent = capture_result.duty_percent;
    return true;
}
/* [COPY END: TIMER_CAPTURE] */

int main(void)
{
    SYSCFG_DL_init();
    if (!InitTimerFrequencyMeasurement()) {
        while (true) {
        }
    }
    while (true) {
        if (MeasureTimerFrequency()) {
            /* frequency_hz、duty_cycle_percent 可接显示或控制。 */
        }
        __WFI();
    }
}
