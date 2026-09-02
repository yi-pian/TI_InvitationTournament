# 陌生外部器件 Bring-Up 保姆教程

> 场景：比赛断网后，你拿到一块没用过的板子和一份 Datasheet。目标不是立即写完整驱动，而是安全地完成一个最小、可观察、可重复的功能。

## 0. ELECTRICAL SAFETY CHECK：代码之前先保住硬件

在下列项目没有写出答案之前，**不要通电，也不要接 MSPM0 GPIO**。

| 必查项 | 从哪里找 | 需要写出的结论 |
|---|---|---|
| 模块板供电 | 板上丝印、原理图、模块说明；其次才是芯片 Datasheet | VCC/GND 接哪根线；正常工作电压，不是 Absolute Maximum |
| 数字 IO 电平 | `Digital input characteristics`、`VIH/VIL/VOH/VOL` | 3.3 V 能否被识别为高；器件输出是否会超过 MSPM0 IO 允许范围 |
| 模拟输入范围 | `Input range`、`common-mode`、`absolute max` | 是否允许负压；是否需要衰减、偏置、限流或钳位 |
| 模拟输出范围 | `Output compliance/swing/current` | 能否直接接 ADC、示波器或负载；需不需要缓冲/终端/滤波 |
| 共地或隔离 | 原理图、接口章节 | 必须共地，还是隔离侧不能直接相连 |
| 反接/短路风险 | 输出结构、模块板电路 | 输出能否短接；继电器/电机是否有续流与独立供电 |
| 上电顺序 | `Power-up sequence`、`latch-up`、`power sequencing` | 谁先上电；上电前 GPIO 应为输入、高阻、低还是高 |
| 电流预算 | `Supply current`、背光/线圈/电机参数 | LaunchPad 3.3 V/5 V 是否带得动；是否需要外部电源 |

### 0.1 三个容易烧东西的误区

1. `Absolute Maximum` 是损坏边界，不是推荐工作点。
2. 芯片支持 5 V，不等于模块板每个 IO 都能直接接 3.3 V MCU。
3. 模块板上的 `VCC`、`VIN`、`3V3`、`5V` 可能分别位于稳压器两侧，不能只凭颜色或排针位置猜。

第一次通电建议：可限流电源、低限流值起步、MCU GPIO 先保持输入/高阻、手摸温升之前先断电、示波器地夹只接系统地。任何引脚或供电不明时标记 `DATASHEET REQUIRED` 并停止接线。

## STEP 1：确认它到底是什么

先抄下**完整料号、厂家、封装/板号、板上晶振字样和丝印**。不要把“蓝色 DDS 板”“0.96 OLED”当作型号。

回答五句话：

1. 它把什么输入变成什么输出？
2. MCU 是传数据、发命令，还是只控制开关？
3. 它是裸 IC，还是包含晶振/稳压/电平转换/运放/滤波的模块板？
4. 最小可见功能是什么？例如 DDS 输出固定正弦、ADC 读到稳定码、OLED 点亮一个像素。
5. 验证它需要什么仪器？万用表、示波器、逻辑分析仪、串口打印或已知电压源。

典型转换关系：DDS 是“数字控制→模拟波形”；ADC 是“模拟量→数字码”；DAC 是“数字码→模拟量”；数字电位器是“数字命令→抽头/电阻变化”；模拟开关是“数字选择→模拟通道切换”。

## STEP 2：第一遍 Datasheet 只找 10 个位置

不要从第一页逐字读到最后。用 PDF 搜索：

`features`、`applications`、`recommended operating conditions`、`absolute maximum`、`pin configuration`、`pin description`、`digital input`、`timing characteristics`、`power-up/reset`、`register map`、`application circuit`。

第一遍只记录：

- 正常电源和 IO 电平；
- 引脚功能及方向；
- 接口类型；
- 最大/最小时钟和关键建立保持时间；
- Reset、上电默认状态；
- 第一个要写/读的寄存器或 1 个最小控制字；
- 一张最接近你用途的典型电路。

更完整的勾选表见 [`DATASHEET_QUICK_CHECKLIST.md`](DATASHEET_QUICK_CHECKLIST.md)。

## STEP 3：把所有 Pin 分类，不要一上来就接线

做一张表：

| Pin | 方向 | 分类 | 上电状态/要求 | 接 MSPM0 还是外部电路 | 依据页码 |
|---|---|---|---|---|---|
| VCC/GND | 电源 | Power | 电压与时序 | 电源 | |
| CLK/SCL/SCK | MCU→器件 | Clock | 空闲极性、最大频率 | SPI/I2C/GPIO | |
| SDI/MOSI/DATA | MCU→器件 | Data | 采样边沿 | SPI/GPIO | |
| SDO/MISO | 器件→MCU | Data | 三态条件、输出电平 | SPI/GPIO | |
| CS/SYNC/LE/LDAC | MCU→器件 | Control | 有效高/低 | GPIO/SPI CS | |
| RESET/PD/EN | MCU→器件 | Control | 有效电平、脉宽 | GPIO | |
| DRDY/BUSY/INT | 器件→MCU | Status | 边沿/电平、是否开漏 | GPIO/IRQ/Capture | |
| AIN/AOUT/REF | 模拟 | Analog | 范围、阻抗、负压 | 模拟前端/ADC/DAC | |
| NC/EP | 特殊 | Special | 悬空、接地或散热 | 严格按 Datasheet | |

模块板和裸 IC 分开做表。模块排针可能只引出部分 IC Pin，还可能改名。

## STEP 4：识别接口，不要看到三根线就叫 SPI

- 有 `SCL/SDA`、地址、ACK、START/STOP：通常是 I2C。
- 有 `SCLK/MOSI/MISO/CS`：通常是标准 SPI，但仍要确认 Mode、位宽和 CS 行为。
- 有 `W_CLK/FQ_UD/DATA`、`INC/U/D/CS` 等自定义名称：往往是同步串行或脉冲协议，可能不能直接套标准 SPI。
- 有 `TX/RX/baud/start/stop/parity`：UART。
- 有 `D0..D15`、`RD/WR/CS/BUSY`：并口/外部总线风格。
- 只有选择线、使能线、脉冲线：GPIO 控制。

完整接口解释见 [`EXTERNAL_INTERFACE_BEGINNER_GUIDE.md`](EXTERNAL_INTERFACE_BEGINNER_GUIDE.md)。

## STEP 5：把 Timing Diagram 翻译成自己的话

不要只截图。至少写出以下句子：

1. 空闲时 Clock 是高还是低？
2. 数据在上升沿还是下降沿被器件采样？MCU 应在哪个边沿前改变数据？
3. CS 何时有效；一帧中能不能抬高；每个字还是整串命令后才抬高？
4. 每个字多少位，MSB-first 还是 LSB-first？
5. 写入后靠 CS、LE、LDAC、FQ_UD 还是某寄存器提交？
6. Reset 有效电平、最小脉宽、释放后等待多久？等待按时间还是参考时钟周期？
7. 读操作何时返回数据；第一字节是不是 dummy；BUSY/DRDY 何时有效？

把 Datasheet 符号换成约束：`tSU >= ...`、`tH >= ...`、`fSCLK <= ...`。第一次 Bring-Up 把速率设得明显低于上限，但不能违反最小脉宽或超时规则。

## STEP 6：确定 SysConfig 里真正需要什么

1. 先决定接口：GPIO、SPI、I2C、UART、Timer/PWM、Capture/IRQ 或并口 GPIO。
2. 再决定候选 Pin，检查是否和 ADC、DAC、TFT、调试串口冲突。
3. 在 SysConfig 添加对应外设，设置实例、PinMux、时钟、Mode、位宽、速率和中断（仅当最小实验真的需要）。
4. Generate 后只使用生成的实例名与 Pin 宏；不要改生成的 `ti_msp_dl_config.c/.h`。

MSPM0 页面怎么点、怎样查 PinMux/Clock/Event/DMA，请按 [`../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md`](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md) 操作。本教程不会重复猜写某个 SDK 版本的下拉框名称。

SysConfig Generate 后，如果不知道 `DL_GPIO_setPins`、`DL_SPI_transmitDataBlocking8`、`DL_I2C_startControllerTransfer`、`DL_UART_Main_transmitDataBlocking` 等函数怎样使用，请查 [`../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md`](../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)；现场速查用 [`../../00_docs/TI_DRIVERLIB_QUICK_REFERENCE.md`](../../00_docs/TI_DRIVERLIB_QUICK_REFERENCE.md)。两份文档解释 MCU 侧 API，片外器件的 SPI mode、I2C 地址、命令和时序仍以该器件 Datasheet 为准。

### 6.1 第一次 Bring-Up 的接口选择

- 普通 SPI/I2C：先 Blocking、低速、轮询状态。
- UART/GPIO：同样先做一个可观察的 byte 或电平动作；DriverLib 参数和 blocking/non-blocking 区别按上述 DriverLib 指南核对。
- 特殊低速串行：可以 GPIO bit-bang，把每根线和边沿看清楚。
- 高速 ADC/DAC：按硬件 SPI/并口/Timer/DMA 规划，bit-bang 只用于确认少量配置寄存器，不用于正式数据流。
- DRDY/BUSY：最初可以轮询 GPIO；稳定后再改中断或 Event/DMA。

## STEP 7：先写最小硬件自检，不写业务功能

优先级从易观察到难观察：

1. 读取固定 ID/版本寄存器；
2. 写一个可回读的配置位，再读回；
3. 执行 Reset，确认状态恢复；
4. 产生一个固定、容易测量的输出；
5. 对 ADC 输入 GND/VREF/已知直流；
6. 最后才做连续采样、波形刷新、扫频或 DMA。

最小程序应只有 `Board_Init → Device_Init → One_Action → Check_Result`。先不要加 FFT、菜单、屏幕动画或大状态机。

## STEP 8：严格执行 Power-Up / Reset

从资料写出一条时间线：

```text
GPIO 先为高阻/安全电平
→ 模块供电稳定
→ 等待 datasheet 指定时间
→ 硬件 Reset 脉冲
→ 等待恢复时间/时钟周期
→ 写完整初始控制字
→ 清状态/读 ID
→ 开始最小功能
```

如果 Reset 不清输入寄存器，第一次写入必须覆盖整字，不能依赖未知上电值。若资料警告“器件上电前禁止数字输入”，先让 MSPM0 Pin 保持输入，等器件供电稳定后再切为输出。

## STEP 9：读 Register Map 时按五列记录

| Address/Command | Reset value | R/W | 目标位 | 其他位怎样保存 |
|---|---:|---|---|---|
| | | | | |

写寄存器的安全做法：

```c
old_value = device_read(REG_X);
new_value = (old_value & ~FIELD_MASK) | FIELD_ENCODE(wanted);
device_write(REG_X, new_value);
```

只有 Datasheet 明确写“整寄存器均可直接写”时才用常量覆盖。特别留意 `reserved`、`write-one-to-clear`、`self-clearing`、只写、读清零、页选择和多字节顺序。

## STEP 10：把驱动分成两层

```text
Application
    ↓ 频率/电压/通道/增益等业务参数
Device core：编码命令、寄存器、换算、状态机
    ↓ write/read/gpio/delay 回调
MSPM0 platform：DL_SPI/DL_I2C/DL_GPIO/Timer 与 SysConfig 实例
    ↓
External device
```

核心层不应到处出现 `DL_SPI_xxx`。平台层只负责“怎样把字节或电平送出去”。不要为了抽象而创建十层；对初次启用，`init`、`read/write`、`set主要参数`、`get状态` 足够。

## STEP 11：按器件类别做最小验证

| 类别 | 最小动作 | 怎样判断 PASS |
|---|---|---|
| DDS/PLL | 输出一个低于参考时钟很多的固定频率 | 示波器/频率计读数正确；更新频率后随之变化 |
| DAC | 写 0、半量程、接近满量程三个码 | 万用表读数单调且接近理论值，不超模拟范围 |
| ADC | 测 GND、已知中点、已知高电平 | raw 码稳定、单调、饱和方向正确 |
| OLED/TFT | 清屏、点单点、画两种颜色/文字 | 方向、颜色、地址范围正确，无随机复位 |
| 编码器/按键 | 慢速动作并打印状态 | 每格方向正确；抖动可解释；长按不乱跳 |
| MUX/Relay | 固定选择每个通道 | 万用表/已知信号证明确实切换，未同时导通 |
| 数字电位器/PGA | 最小、中间、最大三档 | 阻值/增益单调；端点和模拟范围未越界 |
| 存储器 | 写固定模式、掉电/复位后读回 | 地址、页边界、擦写约束符合资料 |

逻辑分析仪先看数字协议，示波器再看模拟结果。若通信正确而模拟结果错，把“数字层”和“模拟层”分别排查。

## STEP 12：只有 Bring-Up PASS 后才接入比赛工程

接入前保存这些证据：

- 实际模块板正反面照片/板号；
- Datasheet 版本与关键页；
- 最终接线表；
- SysConfig 资源与 Pin；
- 最小程序和逻辑分析/示波器结果；
- 正常供电电流；
- 已知限制。

然后才做：换成应用 API、加入错误处理、必要时中断/DMA、连接信号算法链、更新资源表。应用应链接正式驱动，不能把驱动内部几百行复制进 `main.c`。

## 失败时按层定位

| 现象 | 先查 |
|---|---|
| 一通电就限流/发热 | 供电极性、短路、电压、模块板 VIN/VCC 区别；立即断电 |
| 数字线完全不动 | SysConfig PinMux、实例宏、GPIO 方向、代码是否真的执行 |
| 有 Clock 但器件不答 | CS/地址、SPI Mode、位序、Reset、上电顺序、IO 电平 |
| 寄存器能读但输出不变 | Update/LDAC/EN、输出级、参考源、功耗模式、模拟负载 |
| 偶尔正确 | 建立保持时间、线长、地、去耦、上拉、并发访问、BUSY/DRDY |
| DMA 后失败、Blocking 正常 | 请求源、数据宽度、地址自增、缓冲长度、缓存/竞态 |

## 最后判定

只有同时满足“电气安全已确认、最小通信可重复、主要功能可测、失败现象可定位”，才写 `BRINGUP PASS`。Compile 通过不是板级验证；没有板测证据不得写 `BOARD_VERIFIED`。
