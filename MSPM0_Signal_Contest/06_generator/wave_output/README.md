# Wave Output 整合模块

遵循 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)。

## 什么时候用

当题目要求 MSPM0G3507 用片上 DAC 连续输出正弦波、方波、三角波或锯齿波，并希望
`main.c` 只填写频率、峰峰值和偏置时使用。只输出固定直流时直接用 DAC；捕获波形回放
应使用 Arbitrary Wave；没有完成 DAC/Timer/Event/DMA SysConfig 闭环时不要调用本模块。

## 输入和输出

- 初始化输入：调用者提供静态 `wave_table`、静态 `output_buffer`、各自容量、DAC DMA
  配置、DAC 位数和参考电压。这两个数组在 DMA 工作期间必须一直有效。
- 每次输出输入：`frequency_hz`（Hz）、`vpp_v`（Vpp）、`offset_v`（V）。
- 方波扩展输入：`duty_fraction`（0～1 的小数，占空比；`0.5f`=50%）。
- 锯齿波扩展输入：`symmetry_fraction`（0～1 的小数，上升段占周期比例；`1.0f`=标准上升锯齿，`0.5f`=上升/下降各半周期）。
- 硬件输出：DAC 引脚上的连续周期波形。
- 软件结果：`SignalWaveOutput_GetLastResult()` 返回请求/实际频率、Vpp、偏置和 DMA 点数。

## 模块链

```text
Sine / Square / Triangle / Sawtooth
-> DAC Wave Table Validate
-> DDS 整周期 Fill
-> DAC DMA
-> DAC OUT
```

## 作用

把波形表、正弦/方波/三角波/锯齿波、DDS 和 DAC DMA 整合为四个基础三参数接口。
方波和锯齿波另提供带形状参数的四参数接口。调用后自动停止旧输出、生成波表、按整数周期填充 DMA、启动循环输出。

## 依赖

`signal_sine/square/triangle/sawtooth`、`signal_dac_wave_table`、`signal_dds`、`signal_dac_dma_mspm0g3507` 及其依赖必须一起复制到工程 `modules`。

## SysConfig

本封装不新增外设，但它调用 DAC DMA，因此【需要 SysConfig】。严格按下面步骤配置：

1. 双击工程 `.syscfg`，添加 `DAC12/DAC0`，开启 Analog Output、FIFO 和 DMA trigger，
   硬件触发选择 `HWTRIG0`。MSPM0G3507 示例输出为 PA15，不能任意换成普通 GPIO。
2. 添加 DMA，实例名必须与底层 README 一致为 `SIGNAL_DAC_DMA`；选择一个未冲突通道，
   Source/Destination Length 均为 Half Word，地址方向为 block-to-fixed，模式为
   `FULL_CH_REPEAT_SINGLE`。
3. 添加周期 Timer，实例名 `SIGNAL_DAC_TIMER`；示例使用 TIMG6、BUSCLK、Divider=1、
   Prescaler=1、Periodic Down Counting。100 kSPS 对应期望周期 10 us。
4. 在 Timer Event Configuration 中发布 ZERO event，并把相同 Event channel 接到 DAC
   `HWTRIG0` subscriber；示例 channel=3，若工程已有占用必须换成无冲突通道并保持两端一致。
5. 开启 DAC DMA-done interrupt，保存并 Generate；核对 `SIGNAL_DAC_DMA_CHAN_ID`、
   `SIGNAL_DAC_TIMER_INST`、Timer `*_LOAD_VALUE`、DAC instance 和 IRQ 宏均已生成。
6. 检查 PA15、DMA、TIMG6、Event channel 不与 ADC、屏幕或其他模块冲突，再 Clean Build。

以上内容与 `06_generator/dac_dma/README.md` 一致；若本工程实例或通道不同，只允许在
SysConfig 中按实际资源调整，并同步使用生成宏，不能修改 `ti_msp_dl_config.c/.h`。
`dac_config.update_rate_hz` 必须填写 Timer 的真实更新率。

## 最小示例

```c
#include "signal_wave_output_mspm0g3507.h"
static uint16_t wave_table[256];
static uint16_t output_buffer[512];
static const signal_wave_output_config_t config = {
    wave_table, 256U, output_buffer, 512U,
    {100000U, CPUCLK_FREQ, 65536U}, 12U, 3.3f
};

void App_Start(void)
{
    if (SignalWaveOutput_Init(&config) != SIGNAL_RESULT_OK) return;
    if (SignalWaveOutput_SineWithOffset(
            1000.0f,   /* 频率 Hz */
            1.0f,      /* 峰峰值 Vpp */
            1.65f) !=  /* 直流偏置 V */
        SIGNAL_RESULT_OK) return;
}
```

三个参数分别是：频率 Hz、峰峰值 V、直流偏置 V。必须满足 `offset±Vpp/2` 在 0～参考电压范围内。模块会返回请求频率对应的整数周期实际频率，可通过 `SignalWaveOutput_GetLastResult` 读取。

## 其他调用

```c
SignalWaveOutput_SquareWithOffset(frequency_hz, vpp_v, offset_v);
SignalWaveOutput_SquareWithDuty(frequency_hz, vpp_v, offset_v, 0.5f);
SignalWaveOutput_TriangleWithOffset(frequency_hz, vpp_v, offset_v);
SignalWaveOutput_SawtoothWithOffset(frequency_hz, vpp_v, offset_v);
SignalWaveOutput_SawtoothWithSymmetry(frequency_hz, vpp_v, offset_v, 1.0f);
SignalWaveOutput_Stop();
```

## API

- `SignalWaveOutput_Init(config)`：初始化一次 DAC DMA，并保存波表、缓冲区和量程配置。
- `SignalWaveOutput_SineWithOffset(frequency_hz, vpp_v, offset_v)`：输出正弦波。
- `SignalWaveOutput_SquareWithOffset(...)`：输出 50% 占空比方波。
- `SignalWaveOutput_SquareWithDuty(...)`：按 `duty_fraction` 输出方波；参数必须严格在 0～1 之间。
- `SignalWaveOutput_TriangleWithOffset(...)`：输出三角波。
- `SignalWaveOutput_SawtoothWithOffset(...)`：输出上升锯齿波。
- `SignalWaveOutput_SawtoothWithSymmetry(...)`：按 `symmetry_fraction` 设置上升段比例；参数范围为 `0 < x <= 1`。
- `SignalWaveOutput_GetLastResult(result)`：读取上一轮成功输出的实际参数。
- `SignalWaveOutput_Stop()`：主动停止 DAC DMA；正常连续播放无需调用。
- `SignalWaveOutput_GetModuleStatus()`：读取模块验证成熟度，不启动硬件。

## 常见错误

- 返回 `OUT_OF_RANGE`：偏置加减半个 Vpp 超出 0～参考电压，或低频所需点数超过缓冲区。
- 没有输出：DAC FIFO、DMA trigger、Timer Event、DAC subscriber 中至少一环没有接通。
- 频率有小误差：这是整数周期 DMA 的量化结果，读取 `actual_frequency_hz`，不是残影故障。
- 高频像阶梯或锯齿：每周期 DAC 点数太少；提高更新率或加模拟重建低通。
- 切换参数无效：必须再次调用对应 `...WithOffset()`，只改 main 变量不会改正在播放的缓冲区。
- 形状参数无效：`duty_fraction` 和 `symmetry_fraction` 不是百分数整数；方波范围为 `0 < x < 1`，锯齿对称度范围为 `0 < x <= 1`。

## 注意

- 只在 `SYSCFG_DL_init()` 之后调用一次 `SignalWaveOutput_Init()`；以后切换频率、幅度、
  偏置或波形时，直接调用四个 `...WithOffset()` 函数之一。
- `vpp_v` 确实是峰峰值，不是峰值；必须满足 `offset_v - vpp_v/2 >= 0` 且
  `offset_v + vpp_v/2 <= reference_voltage_v`。
- 模块用 `round(update_rate/frequency)` 得到 DMA 点数，再以该点数生成恰好一个完整周期，
  因而循环缓冲首尾连续。请求频率不能被整数点数精确表示时，实际频率会有小量误差，
  用 `SignalWaveOutput_GetLastResult()` 读取。
- 最低频率受 `output_capacity` 限制：大致要求
  `frequency_hz >= update_rate_hz/output_capacity`；最高频率还受每周期点数和 DAC 建立时间限制。
- 输出频率越高，每周期 DAC 点数越少；正弦输出端建议增加重建低通滤波器。
- 旧的方波 `...WithOffset()` 接口使用 50%；旧的锯齿 `...WithOffset()` 接口使用 100% 标准上升锯齿，因此已有工程无需修改即可保持原波形；需要调节时改用带 `Duty` 或 `Symmetry` 后缀的新接口。
- 模块不修改底层 DDS 或 DAC DMA 文件；它只是比赛用的上层闭环封装。
