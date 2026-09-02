# 90_dds_usage

## 推荐复制函数

`InitDDSOutput() + SetDDSFrequency()`（COPY 区 `DDS_INIT`、`DDS_SET_FREQUENCY`）；键盘/串口调整使用 `HandleDDSFrequencyAdjust()`。输入/输出统一为 `frequency_hz`（Hz）。需要在正弦、方波、三角波、锯齿波之间切换时，统一调用 `SignalWaveOutput_Start()`，不要在主程序里重复四套底层逻辑。

## 1. 这个工程干什么

复用现有 Wave Output 整合模块，完成波表、DDS、DAC DMA 的正弦输出和频率更新。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| 初始化 DDS/DAC | `DDS_INIT` |
| 固定频率输出 | `DDS_FIXED_FREQUENCY` |
| 修改频率 | `DDS_SET_FREQUENCY` |
| 按键调频接入点 | `DDS_KEY_ADJUST` |

## 3. 输入

`frequency_hz`、wave table、DAC output buffer；均与 `signal_wave_output_mspm0g3507` 的真实配置结构对应。

## 4. 输出

DAC DMA 波形和 `dds_result`。

## 5. 公共数据链

`wave table → DDS integer-cycle buffer → DAC DMA`。

## 6. 功能与 COPY 区对应表

必须先复制 `DDS_INIT`；固定/修改频率二选一或同时使用。

## 7. 使用的模块

`signal_wave_output_mspm0g3507` 及其 DDS、波表、DAC DMA 依赖；调用依据 restored example04 `App_ApplyWaveform` 和真实头文件。

独立复制文件清单：`signal_status.h`、`signal_dac_dma_mspm0g3507.c/.h`、`signal_dac_wave_table.c/.h`、`signal_dds.c/.h`、`signal_sine.c/.h`、`signal_square.c/.h`、`signal_triangle.c/.h`、`signal_sawtooth.c/.h`、`signal_arbitrary_wave.c/.h`、`signal_wave_output_mspm0g3507.c/.h` 与仅头文件 `signal_math.h`。`signal_math.h` 是波表和正弦实现的直接编译依赖，必须一并复制。

## 8. SysConfig / 引脚

复制 restored example04 DAC/DMA/Timer 配置；不更换 DAC 或 DMA 实例。

## 9. main.c 流程

初始化整合模块，调用正弦输出 API，修改 `frequency_hz` 后重新调用。

## 10. 每个 COPY 区说明

频率变量改变不会自动改正在播放的 DMA 缓冲，必须执行 `DDS_SET_FREQUENCY`。

## 11. 如何复制到新工程

复制 Wave Output 及其依赖、`DDS_INIT` 和需要的输出区，保留 DAC SysConfig。

## 12. 可调参数

频率、Vpp、偏置、波表和输出缓冲容量、DAC 更新率。

统一接口如下：

```c
SignalWaveOutput_Start(type, frequency_hz, vpp_v, offset_v, shape_fraction);
```

- `type` 使用 `signal_wave_output_type_t`；
- 方波的 `shape_fraction` 是占空比，例如 `0.25f` 表示 25%；
- 锯齿波的 `shape_fraction` 是对称度，`1.0f` 是标准上升锯齿波，`0.5f` 接近三角波；
- 正弦波和三角波忽略 `shape_fraction`，统一传 `0.5f` 即可；
- 输出缓冲容量必须不小于 `DAC更新率 / 最低频率`。例如 100 kHz 更新率、最低 100 Hz 时，至少需要 1000 点，推荐分配 1024 点。

## 13. 常见错误

只改变量不重启输出；DAC 更新率不足；输出超出参考电压。

## 14. 本工程没有做什么

不实现外部 DDS 芯片，不自动引入 keypad 工程。

## 15. Build 状态

SysConfig 1.28 Generate、TI Arm Clang 5.1 Compile/Link 已通过；实板 `NOT_RUN`。
