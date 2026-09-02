# example02 步骤 02：统一波形输出模块（DDS 与 DAC DMA）

本阶段已把原来 main 中分散的“波表生成、DDS 填充、DAC DMA 启动”整理成集成库
`wave_output` 模块。比赛现场只需要按模块 README 初始化一次，之后用三个参数调用
`SignalWaveOutput_SineWithOffset(频率_Hz, 峰峰值_V, 偏置_V)`。

## 一、模块 README 复制内容

复制集成库 `06_generator/wave_output` 的 `signal_wave_output_mspm0g3507.c/.h`、
`README.md`、最小示例和完整示例；同时按该 README 的依赖清单复制
`signal_dac_wave_table`、`signal_dds`、`signal_dac_dma_mspm0g3507`、
`signal_sine`、`signal_square`、`signal_triangle`、`signal_sawtooth` 的 `.c/.h`。

## 二、SysConfig 操作

在 `Software -> Add` 中加入 DAC12、TIMG6、DMA 和 Event。DAC12 选择 SysConfig 可用的
DAC 输出引脚；TIMG6 产生 `SIGNAL_DAC_UPDATE_RATE_HZ=100000U` 的更新节拍；Event 将
TIMG6 publisher 接到 DAC DMA；DMA 选择 `DMA_CH2`，避免和双 ADC 的 CH0/CH1 冲突。保存
并 Generate，核对生成宏，不编辑 `ti_msp_dl_config.c/.h`。

## 三、main 中复制与自写代码

复制的主要闭环现在是：

```c
(void)SignalWaveOutput_SineWithOffset(
    g_frequency_hz[g_point], 2.97f, 1.65f);
```

逐行解释：`g_frequency_hz[g_point]` 取当前扫频点的频率；`2.97f` 是峰峰值，原程序
使用 `0.45 × 3.3 V` 的峰值，因此 Vpp 为 `2 × 1.485 V = 2.97 V`；`1.65f` 是直流
偏置，使输出摆幅为 0.165～3.135 V；函数内部按 README 顺序停止旧 DMA、生成波表、
校验波表、计算 DDS 整周期缓冲并启动循环 DMA，main 不再自己拼接底层链路。

初始化区仍需按 `wave_output` README 建立波表、DMA 输出缓冲和
`signal_wave_output_config_t`；这些数组和配置是 main 的少量自写内容，底层波形、DDS、
DAC DMA 文件没有修改。SysConfig 仍完全沿用 DAC DMA README 的 DAC、定时器、DMA、事件
配置，本封装不新增外设。当前实际输出脚、幅值和 DMA 时序仍需上板确认。
