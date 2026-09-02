# 什么时候直接 DriverLib，什么时候使用模块

> 这是判断细则；比赛功能的五类总入口见 [CONTEST_IMPLEMENTATION_GUIDE.md](CONTEST_IMPLEMENTATION_GUIDE.md)。板外芯片应进入 External Device Driver/Bring-Up，不归入片上 DriverLib 与 Signal Module 二选一。

## 一页结论

```text
简单硬件动作 → SysConfig + TI DriverLib
复杂硬件流程 → 正式 Hardware / Integration Module
信号处理     → MSPM0_Signal_Contest
少量纯计算   → Optional Helper（确实省事时才用）
```

本仓库锁定 MSPM0G3507、CCS、SysConfig 和 MSPM0 SDK，不再为跨 MCU 统一接口支付 callback/adapter 成本。

## 10 秒判断法

满足下面大多数条件，直接 DriverLib：

- SysConfig 已完成 PinMux、时钟和静态外设配置；
- 运行时只有 1～5 行 `DL_xxx()`；
- 没有 DMA/Event 多外设协作；
- 没有 buffer 所有权、状态机、复杂时序或数据算法；
- 加 wrapper 后 struct、callback、source/include 反而更多。

出现下面任一项，优先找正式模块：

- Timer + Event + ADC/DAC + DMA 联动；
- Ping-Pong/Ring Buffer、帧 ready/release、触发前历史；
- ISR 时间戳、回绕、超时、协议状态机；
- 显示器初始化/绘图/字库、矩阵键盘扫描/消抖；
- FFT、RMS、THD、滤波、插值、校准等真实计算。

## 真实例子

| 需求 | 默认方式 | 为什么 |
|---|---|---|
| DAC 输出固定 `2048U` | `SYSCFG_DL_init()` → `DL_DAC12_output12(DAC0, 2048U)` | 一行运行时动作，无状态/buffer |
| GPIO 拉高/读电平 | `DL_GPIO_setPins` / `DL_GPIO_readPins` | DriverLib 已清楚表达动作 |
| Timer 启停/读 counter | `DL_TimerG_startCounter/stopCounter/getTimerCount` | wrapper 不会减少步骤 |
| UART 发少量调试文本 | 循环 `DL_UART_Main_transmitDataBlocking` | 没有协议或异步缓冲需求 |
| ADC 读一个样本 | software trigger + `DL_ADC12_getMemResult` | bring-up 路径简单 |
| ADC 固定 Fs 采 N 点 | ADC DMA 模块 | Timer/Event/ADC/DMA 与 buffer 完成条件复杂 |
| 连续 ADC 双缓冲 | ADC Ping-Pong DMA | CPU 与 DMA 的 buffer 所有权需要状态管理 |
| 硬件捕获测频 | Timer Capture | ISR、计数方向、回绕、超时、ticks→Hz 值得复用 |
| DAC 连续输出正弦 | Sine/Wave Table/DDS → DAC DMA | 生成算法与 Timer/Event/DMA/DAC 都有实际价值 |
| 4×4 键盘 | Matrix Keypad 4×4 | 行列扫描、settle、映射、消抖不是一行 GPIO |
| ILI9341 显示 | TFT ILI9341 | 命令协议、时序、绘图和字库复杂 |
| Vpp/RMS/FFT/THD | Algorithm Module | 这是信号计算，不是硬件寄存器动作 |

## Optional Helper 的边界

Helper 应当是纯计算：不持有硬件 instance，不调用 callback，不要求 Platform/BSP/SysConfig。例如目标 Hz → Timer count、标称/实测 VREF 选择、OPA/ADC 电压余量计算。

如果 helper 本身需要 descriptor + callback 才能写一个寄存器，它已经越界。现有 `SignalDAC_VoltageToRaw()` 有纯换算价值，但与旧 DAC BSP 同目录，本轮只保留兼容，不把它包装成新的推荐硬件链，也不复制第二份换算源码。

## SysConfig 与 DriverLib 的分工

| SysConfig 负责 | DriverLib/模块在运行时负责 |
|---|---|
| PinMux、方向、上下拉、初值 | set/clear/read/toggle GPIO |
| 时钟、divider、peripheral 静态参数 | start/stop/read/status |
| ADC channel/MEM/reference/trigger | 单次 start/get result；N 点用 ADC DMA |
| DAC reference/output/FIFO/event | 固定 code 直接 write；连续流用 DAC DMA |
| Event Fabric、DMA channel/trigger | 复杂模块按既定资源启动/停止 |

## 遇到旧 BSP / Mega Platform 时

`01_bsp/adc|dac|dma|gpio|timer|uart|comparator` 与 `08_applications/common/mspm0g3507/signal_mspm0g3507_platform.c` 暂时保留，避免旧工程突然断链。它们不是新项目的必经层。

新项目先从 [CONTEST_IMPLEMENTATION_GUIDE.md](CONTEST_IMPLEMENTATION_GUIDE.md) 判断类型；简单动作再看 [TI_DRIVERLIB_BEGINNER_GUIDE.md](TI_DRIVERLIB_BEGINNER_GUIDE.md)，复杂链按模块 README，纯计算按算法 README，板外芯片按 External Device Bring-Up。
