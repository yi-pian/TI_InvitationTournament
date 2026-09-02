#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_dual_adc_mspm0g3507.h"

// ============================================================
// 用户通常只需要修改这里
// ============================================================

// 每一路 ADC 每秒采样 100000 次，即 100 kSPS。
// 最高输入频率变化时，优先修改它；观察波形通常取最高频率的 10~50 倍。
#define SIGNAL_SAMPLE_RATE_HZ  (100000U)

// 每轮每一路采集 1024 个点。N 增大，观察时间更长、RAM 也更多。
// 两路 raw 缓冲区共占 4 * N 字节；1024 点共占 4096 字节。
#define SIGNAL_SAMPLE_COUNT    (1024U)

static uint16_t g_raw_a[SIGNAL_SAMPLE_COUNT];
static uint16_t g_raw_b[SIGNAL_SAMPLE_COUNT];
volatile signal_result_t g_status;

int main(void)
{
    // timer_clock_hz 必须填写 SysConfig 中采样 Timer 的实际输入时钟。
    // PROFILE_02_DUAL_ADC 使用 BUSCLK/1/1，故这里可用 CPUCLK_FREQ。
    // 65536U 是 16 位 Timer 可表示的最大周期数，初学者通常不要修改。
    const signal_dual_adc_config_t config = {
        .sample_rate_hz = SIGNAL_SAMPLE_RATE_HZ,
        .timer_clock_hz = CPUCLK_FREQ,
        .timer_max_count = 65536U,
    };

    // 先由 SysConfig 初始化 Timer、两路 ADC、DMA 和 Event。
    SYSCFG_DL_init();

    // 模块初始化只需在上电后执行一次。
    g_status = SignalDualADC_Init(&config);
    if (g_status != SIGNAL_RESULT_OK) while (1) { }

    while (1) {
        // 启动本轮两路同步 ADC + DMA 采集。
        g_status = SignalDualADC_Start(
            g_raw_a, g_raw_b, SIGNAL_SAMPLE_COUNT);
        if (g_status != SIGNAL_RESULT_OK) while (1) { }

        // 等待两路 DMA 都完成；等待期间 CPU 进入低功耗等待中断状态。
        while (!SignalDualADC_IsFinished()) { __WFI(); }

        // ===== 从这里开始写自己的信号处理逻辑 =====
        // g_raw_a[i] 与 g_raw_b[i] 对应同一次 Timer 触发的两个 ADC 原始码。
        // 可分别送入 ADC To Voltage，再连接相位、RMS、FFT 或峰峰值模块。
    }
}
