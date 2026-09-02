# 外部器件接口保姆教程

本文件只回答：“Datasheet 里的接口怎样映射到 MSPM0G3507 和 SysConfig？”具体器件的电压、Mode、地址、引脚仍以其官方资料和手上模块板为准。

## 1. 先用这张表认接口

| 接口 | 常见信号名 | MCU 做什么 | SysConfig 通常需要 |
|---|---|---|---|
| GPIO | EN、RESET、CS、SEL、BUSY、INT | 输出固定电平/脉冲，或读取状态 | GPIO PinMux；必要时 Interrupt |
| SPI | SCLK、MOSI/SDI、MISO/SDO、CS/SYNC | 同步移位一个或多个字 | SPI Controller + CS/GPIO |
| I2C | SCL、SDA、ADDR | 带地址、ACK 的两线总线 | I2C Controller + 开漏/上拉 |
| UART | TX、RX、RTS、CTS | 按波特率发送异步字节 | UART + baud/frame |
| Parallel | D0..Dn、RD、WR、CS、BUSY | 多根 GPIO 同时表示一个字 | GPIO 组；可能 Timer/IRQ/DMA |
| PWM | PWM、CTRL、DIM | 用占空比/频率表示控制量 | Timer PWM |
| Capture | OUT、TACH、FREQ、DRDY | 测输入边沿时间 | Timer Capture 或 GPIO IRQ |
| Interrupt | INT、ALERT、DRDY | 器件提醒 MCU 处理 | GPIO input + edge/level IRQ |
| Analog control | VCTRL、VSET、REF | 用电压控制增益/频率/电流 | 内部 DAC、PWM+RC 或外部 DAC |

## 2. GPIO：最简单，也最容易忽略默认状态

GPIO 不是“接上就行”。每根线要确定：方向、上电默认电平、有效高/低、推挽/开漏、是否上拉、最大翻转速度。

### 输出线

- `RESET_n`：名字带 `_n`、`/RESET` 通常低有效，但必须核对 Datasheet。
- `EN/PD`：Enable 和 Power-down 可能刚好相反。
- `CS/SYNC/LE/LDAC`：往往参与协议时序，不能随意在函数之间翻转。
- 上电前不应驱动的器件：SysConfig/初始化早期保持输入或安全电平，供电稳定后再切输出。

### 输入线

- `BUSY/DRDY/INT` 要确认是高有效、低有效、脉冲还是保持电平。
- 开漏输出必须有上拉；模块板可能已经带上拉，裸 IC 通常没有。
- 第一次 Bring-Up 可轮询，稳定后再使用 GPIO Interrupt。

### SysConfig 思路

添加 GPIO、分配 Pin、设置输入/输出与初始电平。生成后使用 `ti_msp_dl_config.h` 里的实例和 Pin 宏，不把 PAxx/PBxx 数字写死进上层器件核心。

## 3. SPI：比赛中最常见的“看起来一样、细节不同”接口

### 3.1 四根线分别做什么

- `SCLK/SCK`：Controller 产生时钟。
- `MOSI/SDI/DIN`：MSPM0 发给器件。
- `MISO/SDO/DOUT`：器件发给 MSPM0；只写器件可能没有。
- `CS/SYNC`：选中器件并界定一帧。很多 DAC/ADC 对 CS 的要求比 SPI Mode 更关键。

### 3.2 CPOL 和 CPHA 怎样从 Timing Diagram 看出来

1. 找 Clock 空闲时的电平：空闲低是 `CPOL=0`，空闲高是 `CPOL=1`。
2. 从空闲电平出发的第一个边沿叫 leading edge，第二个叫 trailing edge。
3. 如果 Datasheet 说在第一个边沿采样，通常 `CPHA=0`；在第二个边沿采样，通常 `CPHA=1`。
4. 最终以“器件在哪个边沿锁存数据”为准，再映射到 SysConfig 的 SPI Mode。

| SPI Mode | CPOL | CPHA | Clock 空闲 | 采样边沿 |
|---:|---:|---:|---|---|
| 0 | 0 | 0 | 低 | 第一个（上升） |
| 1 | 0 | 1 | 低 | 第二个（下降） |
| 2 | 1 | 0 | 高 | 第一个（下降） |
| 3 | 1 | 1 | 高 | 第二个（上升） |

不要只因某博客写“Mode 0”就设置；看官方时序图中的空闲电平、数据改变边沿和采样边沿。

### 3.3 一帧不一定是 8 bit

外部器件可能使用 10、12、16、24、32 或 40 bit。检查：

- MSB-first 还是 LSB-first；
- 命令、地址、数据分别占几位；
- 8-bit SPI 能否分多次发送且 CS 保持有效；
- 字节之间能否有间隔；
- 读数据前是否要发 dummy byte；
- CS 抬高后是提交数据，还是还需要 `LDAC/LE/FQ_UD` 脉冲。

例如 AD9850 的串行口有 Clock 和 Data，但它是 40-bit、LSB-first，并用 `FQ_UD` 更新；它不是直接套用“8-bit Mode 0 + 硬件 CS”就完成。

### 3.4 把 Timing 参数映射到 SysConfig

Datasheet 给：

- `fSCLK(max)`；
- `tCLKH(min)`、`tCLKL(min)`；
- `tCSS/tCSH`（CS 到 Clock、Clock 到 CS）；
- `tDS/tDH`（数据建立/保持）；
- 两帧间隔、转换时间或 BUSY 时间。

你要做：

1. 选择 SPI 时钟源和分频，使实际 bitrate 不高于 `fSCLK(max)`。
2. 确认高/低脉宽也满足最小值；不能只看平均频率。
3. 在传输前后用 GPIO 或驱动 API满足 CS 建立/保持；硬件自动 CS 不满足帧规则时改用普通 GPIO。
4. 传输结束不等于器件操作结束；若有 BUSY/DRDY，等待它或等待 Datasheet 指定时间。

SysConfig 的具体页面、Clock 和 PinMux 见 [`../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md`](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md)。

### 3.5 最小 SPI Bring-Up

```text
CS=inactive
→ 初始化 SPI（低速、正确 Mode/位序）
→ Reset 器件
→ CS=active
→ 发送“读 ID”或一个完整控制字
→ 接收/提交
→ CS=inactive
→ 等 BUSY/DRDY
→ 检查结果
```

逻辑分析仪至少解码：CS、Clock、MOSI；需要读回时再加 MISO。若解码器显示不对，先人工对照边沿，避免解码器 Mode 设置也错。

## 4. I2C：地址、上拉和 ACK 是核心

### 4.1 先确认四件事

1. Datasheet 给的是 7-bit 地址还是已经左移并包含 R/W 位的“8-bit 地址”。SysConfig/DriverLib API 通常使用哪种形式必须看当前 SDK API。
2. `A0/A1/SA0` 地址脚在模块板上接了高、低还是跳线。
3. SCL/SDA 是否已有上拉，上拉电压是多少；不能上拉到超过 MSPM0 IO 的电压。
4. 标准 100 kHz、Fast 400 kHz 或更高模式是否被器件和线长同时支持。

### 4.2 一次典型寄存器读取

```text
START
→ device_address + Write
→ register_address
→ Repeated START
→ device_address + Read
→ receive N bytes（最后一个 NACK）
→ STOP
```

有的器件用 16-bit 寄存器地址，有的写入后需要等待内部 EEPROM 周期。没有 ACK 时按顺序查：电压/共地、上拉、7/8-bit 地址混淆、地址脚、PinMux、速率、上电/Reset。

## 5. UART：先区分逻辑 UART 和物理层

`TX/RX 3.3 V TTL` 可以直连相容 IO；`RS-232` 有正负电压，`RS-485` 是差分，不能直接接 MSPM0 UART Pin，必须经过收发器。

记录：baud、data bits、parity、stop bits、是否反相、换行/帧头/校验。双方交叉接 `TX→RX`、`RX←TX`，并在非隔离场景共地。Bring-Up 先发送固定字节模式 `0x55`，示波器容易确认位宽。

## 6. Parallel：速度高，但 Pin 和时序成本也高

常见于 AD7606 类 ADC：数据总线、`CONVST`、`BUSY`、`CS/RD`、Reset、量程/过采样选择等。先画时间线：启动转换→BUSY 变化→何时允许读→每次 RD 取几个 bit→读几个通道。

不要默认 GPIO 逐位读取就能满足高速率。需计算：

- 一帧通道数 × 每通道位数；
- 目标采样率需要的总线吞吐；
- GPIO 读/写周期、BUSY 中断延迟；
- 是否需要 DMA、并行捕获外设或更合适的串行模式。

## 7. PWM、Capture 和 Interrupt

- PWM 控背光/电机/模拟控制时，频率影响纹波与可听噪声，占空比影响平均值；负载通常不能由 GPIO 直接驱动。
- Capture 用于测频、转速、脉宽，前提是输入电平与边沿已经整形并满足 MSPM0 Pin 限制。
- Interrupt 只做“快速记录/置标志”，不要在 ISR 内刷屏、长延时或执行完整 SPI 事务。

## 8. Analog Control：DAC 电压不是无限驱动力

VGA/PGA、VCO、滤波器可能用 `VCTRL`。先确认控制电压范围、输入阻抗、带宽、单调方向和断电状态。MSPM0 内部 DAC 或 PWM+RC 能否使用，取决于分辨率、纹波、更新速度和缓冲需求；必要时加运放、钳位或外部 DAC。

## 9. “Datasheet → SysConfig”最终映射表

| Datasheet 信息 | SysConfig/软件落点 |
|---|---|
| 引脚功能与复用 | PinMux / Hardware view |
| `VIH/VIL`、开漏 | 电平转换、GPIO output/open-drain、外部上拉，不是纯软件参数 |
| SPI CPOL/CPHA | SPI Mode |
| bit rate / pulse width | Clock source + divider + 软件延时 |
| MSB/LSB、word size | SPI frame/发送函数或 device core 打包 |
| CS/RESET/LDAC | 硬件 CS 或独立 GPIO，通常由 platform 层控制 |
| I2C address | device core 配置，不在 PinMux |
| DRDY/BUSY/INT | GPIO input、Interrupt edge 或 Timer Capture |
| PWM frequency/duty | Timer PWM period/compare |
| DMA request | DMA channel + 真实 peripheral trigger；稳定后再加 |

每改一次配置都 `Generate → Clean → Build`，并检查生成宏是否与 platform 层使用的一致。

