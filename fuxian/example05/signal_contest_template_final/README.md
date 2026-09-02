# Signal Contest Template：比赛复制母版

这是 MSPM0G3507 / CCS / SysConfig 的最小比赛母版。它不预装任何信号模块，但已经默认配置 MSPM0 SDK 自带 CMSIS-DSP 1.16.2；你可以直接 `#include "arm_math.h"`。选好竞赛专用功能后，再按模块 README 把冻结副本复制进 `modules/`。

## 正确使用方法

1. 整个复制/Import 本工程到你的赛题目录。
2. 先判断简单功能是否直接用 SysConfig + TI DriverLib。
3. 确实需要模块时，打开模块 README。
4. 按 README 修改本工程 `signal_contest_template.syscfg`。
5. 把 README 明列的 `.c/.h/.inc` 复制到 `modules/`。
6. 在 CCS Project Explorer 中 Refresh；本题 `.cproject` 已自动排除旧片内 DAC/DDS 演示源，确认它们为 **Excluded from Build**，同时确认 `ad9833.c` 和 `mspm0_blocking_bus.c` 已加入 Build。
7. 把 README 的 include、变量、初始化、处理代码复制到 `main.c` 标记区。
8. 每加一个模块 Build 一次；全部完成后 Clean/Build 和上板验证。

## 从初始母版 `.syscfg` 开始的图形界面路径

初始文件就是本目录的 `signal_contest_template.syscfg`。在 CCS Project Explorer 中双击它，进入 SysConfig 图形界面；所有硬件配置都从左侧 `Software` -> `Add` 添加，不能直接编辑 `.syscfg` 文本或生成的 `ti_msp_dl_config.c/.h`。

按模块 README 操作时统一遵循这条点击顺序：

1. 左侧 `Software` -> `Add` -> 选择外设模块（例如 `ADC12`、`TIMER`、`DMA`、`EVENT`、`SPI`、`UART`）。本题的 AD9833 是外部 SPI 模块，不添加片内 DAC12。
2. 在新建的外设实例顶部设置实例名和硬件 instance；实例名必须与 README 的平台绑定宏一致，硬件 instance 和 PinMux 必须按当前工程冲突检查结果选择。
3. 展开 README 指定的 `Basic Configuration`、`Sampling Mode Configuration`、`ADC Conversion Memory Configurations`、`Clock Configuration`、`Event Configuration`、`DMA Configuration` 或 `PinMux Peripheral and Pin Configuration`，按字段顺序逐项设置。
4. 点击 Generate/保存，回到生成文件只核对实例名、时钟、LOAD、DMA 和 IRQ 宏；任何配置改变后都要重新 Generate、Clean、Build。

例如 ADC 定时 DMA 的入口路径是：`Software` -> `Add` -> `ADC12` -> 实例 -> `Basic Configuration` -> `Sampling Mode Configuration` -> `ADC Conversion Memory Configurations` -> `ADC Conversion Memory 0 Configuration` -> `Input Channel`。然后按对应 README 继续配置 `Reference Voltage`、`Sample Period Source`、`Advanced Configuration`、`Event Configuration`、`DMA Configuration` 和 Timer publisher/subscriber。这里的“路径”是图形界面点击层级，不是要复制到代码里的 DriverLib 名称。

## 母版已经替你处理什么

- `projectspec` 只引用本工程自己的文件和 TI SDK，不依赖仓库根路径变量。
- CMSIS Core/DSP include、`ARM_MATH_CM0` 和 SysConfig `genLibCMSIS` 已配置；不需要手工加库。
- Include Path 已包含 `${PROJECT_ROOT}/modules`。
- `.syscfg` 初始只含 Board/SYSCTL；模块 README 要求什么外设才添加什么。
- `main.c` 已给出模块初始化区、模块调用区和自己的题目逻辑区。
- `signal_config.h` 集中存放常见 Fs/N/VREF/AD9833 MCLK 参数，可按题目删减。
- `COPIED_MODULES.md` 只记录来源，比赛工程无需保持与正式库同步。

## 简单动作与真正模块

| 功能 | 怎么做 |
|---|---|
| GPIO set/clear、读当前按键电平、单点 ADC bring-up、简单 blocking UART/SPI | 直接 SysConfig + TI DriverLib，不复制 Signal Module |
| ADC DMA、Dual ADC、Timer Capture、TFT、外部 AD9833 | 按模块 README 配 SysConfig 并复制比赛版文件 |
| Mean、Min/Max、Vpp、RMS、普通 FFT/FIR/IIR/Correlation | 直接按 `CMSIS_DSP_CONTEST_COOKBOOK.md` 调用 CMSIS，不复制普通核心 |
| 插值、稳健估计、谐波识别、THD、校准等 | 复制对应 Contest-specific Module；底层仍复用 CMSIS 算子 |

## 目录说明

```text
signal_contest_template/
├─ main.c                         你的组合代码
├─ signal_config.h                集中比赛参数
├─ signal_contest_template.syscfg 硬件配置源
├─ modules/                       复制进来的冻结模块
├─ COPIED_MODULES.md              简单来源记录
└─ ticlang/*.projectspec          CCS Import 入口
```

## 母版最小验证

空母版和 `10_tests/cmsis_dsp_contest_smoke/` 共同验证 SysConfig/CCS/CMSIS 工程骨架；每个模块的真实隔离复制结果见 `00_docs/COPY_ASSEMBLY_READINESS.md`。Build 成功不等于新接线已经 `BOARD_VERIFIED`。

## example05 比赛步骤文档

本题按 22_X 的“先模块、再 SysConfig、再复制 main、最后组合逻辑”顺序记录在：

1. [01_模块选择与题目定义.md](01_模块选择与题目定义.md)
2. [03_双路ADC电压电流采集实施步骤.md](03_双路ADC电压电流采集实施步骤.md)
3. [04_ST7789结果显示实施步骤.md](04_ST7789结果显示实施步骤.md)
4. [02_DDS与DAC_DMA激励实施步骤.md](02_DDS与DAC_DMA激励实施步骤.md)（文件名沿用，内容为 AD9833）
5. [05_阻抗计算与自动扫频实施步骤.md](05_阻抗计算与自动扫频实施步骤.md)
6. [06_RLC识别谐振带宽Q实施步骤.md](06_RLC识别谐振带宽Q实施步骤.md)
7. [07_验证与赛场复现实施步骤.md](07_验证与赛场复现实施步骤.md)

模块 README 第 11 节补充了原模块 README 没有的阻抗公式、RLC 拟合和本题 SysConfig
速度差异；所有模块 `.c/.h/.inc` 仍保持冻结未改。
