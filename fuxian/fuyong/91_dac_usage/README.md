# 91_dac_usage

## 推荐复制函数

固定直流输出复制 `SetDACDC()`（COPY 区 `DAC_DC`），输入 `dac_code`（0..4095）。连续波形请复制 `90_dds_usage` 的完整 DDS 函数，而不是在本工程手写 DMA。

## 1. 这个工程干什么

输出固定 DAC 直流码；连续表波引用 `90_dds_usage`，因为两者 SysConfig 链不同。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| 固定 DC 输出 | `DAC_DC` |
| 连续 DAC 表波入口说明 | `DAC_WAVEFORM` |

## 3. 输入

`dac_code`：0..4095 的 12-bit DAC code。

## 4. 输出

DAC0/PA15 直流模拟电压。

## 5. 公共数据链

`SysConfig DAC0 → DL_DAC12_output12(DAC0, dac_code) → PA15`。

## 6. 功能与 COPY 区对应表

DC 用 `DAC_DC`；连续波不要把 P03 DMA 配置硬塞入 P07，直接用 `90_dds_usage`。

## 7. 使用的模块

无旧 DAC DC wrapper；依据 `06_generator/dac_dc/README.md` 推荐的 DriverLib 调用。

## 8. SysConfig / 引脚

复制 `PROFILE_07_BASIC_IO/profile.syscfg`，DAC0、PA15、VDDA/VSSA。

## 9. main.c 流程

初始化后写入一个 DAC code。

## 10. 每个 COPY 区说明

`DAC_WAVEFORM` 是明确跳转说明，不是假装 P07 可以直接 DMA 输出。

## 11. 如何复制到新工程

复制 `DAC_DC` 和 P07 SysConfig；连续输出则改用 P03/90 DDS 工程。

## 12. 可调参数

`dac_code` 及 SysConfig reference/amplifier。

## 13. 常见错误

code 不是伏特；输出范围受参考和负载影响。

## 14. 本工程没有做什么

不做 DMA 及任意波表。

## 15. Build 状态

待统一 SysConfig 生成和 CCS Compile/Link 审计；实板 `NOT_RUN`。
