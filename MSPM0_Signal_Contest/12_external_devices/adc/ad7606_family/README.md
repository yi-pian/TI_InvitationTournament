# AD7606 类同步采样 ADC 通用教程

README 类型：`GENERIC_TUTORIAL`  
实现状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`  
验证状态：没有本目录专用 Driver、没有 Compile 证据、没有上板验证。

这里的“AD7606 类”是架构入口，不等于某个完整料号。AD7606、AD7606B、AD7606C 以及不同通道数、分辨率后缀的器件不能共用一套未经核对的 Pin、时序和数据解析代码。

## 1. 它是干什么的

这类器件把多个 SAR ADC 通道、采样保持、输入前端和数字接口放在一起。最重要的特点是“多个通道在同一时刻采样”，适合三相电、电机、多路幅相或需要比较通道瞬时值的系统。

小白可以这样理解：普通多路 ADC 可能按 CH1、CH2、CH3 依次拍照；同步采样 ADC 像同时按下多台相机的快门。`CONVST` 是快门，`BUSY` 是“照片还在处理”，BUSY 结束后 MCU 才读取这一帧各通道结果。

## 2. 一次采样发生什么

```text
Timer 或 GPIO
      ↓
发出 CONVST（启动转换）
      ↓
全部通道同时采样
      ↓
BUSY 表示正在转换
      ↓
BUSY 进入“转换完成”状态
      ↓
SPI 串行读取，或 CS/RD + DBx 并行读取
      ↓
按器件规定顺序保存 CH1 ... CHx 到 raw[]
```

`raw[]` 只是原始码。正负号、位宽、量程和电压换算都必须按具体型号与硬件 RANGE 设置处理。

## 3. 常见控制信号

| 常见信号 | 通俗作用 | 型号相关点 |
|---|---|---|
| `CONVST` / `CONVSTA/B` | 启动一次或一组通道转换 | 数量、有效边沿和最小脉宽必须查表 |
| `BUSY` / `DRDY` | 告诉 MCU 转换是否完成 | 名称、极性和边沿可能不同 |
| `RESET` | 让器件回到已知状态 | 脉宽、上电等待时间必须查表 |
| `CS` | 选中数字接口 | 串行、并行时职责可能不同 |
| `RD` | 并行模式下读出下一字/通道 | 时序与地址方式按型号确认 |
| `OS[ ]` | 硬件选择过采样档位 | 是否存在、编码、对带宽的影响不同 |
| `RANGE` | 选择模拟输入量程 | 电平含义和允许切换时机不同 |
| `DB0...DBx` | 并行数据总线 | 宽度和码型按型号确认 |
| `SCLK` | 串行读数时钟 | SPI mode 和最高频率按型号确认 |
| `DOUTA/B` / `SDO` | 串行数据输出 | 数据线数量和通道顺序不同 |

这些只是这一类器件常见的功能名称，不保证每个具体型号都存在或同名。

## 4. 拿到实物后先确认什么

| 项目 | 为什么必须确认 |
|---|---|
| 完整型号和后缀 | AD7606 系列成员的能力和 Pin 定义不同 |
| 模块板原理图 | 模块可能加稳压、基准、隔离、缓存或固定绑带 |
| 通道数 | 决定每帧数据量和 `raw[]` 长度 |
| 分辨率 | 决定符号扩展、数据类型和电压换算 |
| 最大采样率 | 决定 Timer 周期、读取带宽和 DMA 需求 |
| 模拟输入范围 | 防止过量程，并决定码值到电压的比例 |
| 数字 I/O 电平 | 判断能否与 MSPM0G3507 直接连接 |
| 串行或并行模式 | 决定使用 SPI 还是 GPIO/总线读取 |
| CONVST 数量与边沿 | 决定同步启动方式 |
| BUSY/DRDY 极性 | 决定轮询或中断条件 |
| OS pins / 软件过采样 | 决定滤波、延迟和有效输出速率 |
| RANGE 控制 | 决定输入范围和换算 |
| 数据码型 | 二补码、偏移二进制等解析完全不同 |
| 数据输出顺序 | 决定 `raw[i]` 对应哪个通道 |

## 5. MSPM0 通常需要什么

### 第一次 Bring-Up

- `GPIO Output`：CONVST、RESET，以及可能的 CS、RD、OS、RANGE。
- `GPIO Input`：BUSY/DRDY；并行模式还需要 DBx 输入。
- `SPI Controller`：仅串行模式需要。
- 串口或屏幕：打印原始码，便于验证。

### 高速连续采样

- `Timer`：提供稳定采样节拍；不要靠软件 delay 当正式采样时钟。
- `DMA`：搬运 SPI RX 或适合的 GPIO/外设数据通路。
- `GPIO Interrupt` 或可用的 Event：响应 BUSY 完成事件。
- Ping-Pong buffer：处理上一帧时继续接收下一帧。

SysConfig 只创建实际硬件所需的 GPIO、SPI、Timer、DMA。先保留生成的名称，再在你的适配层中映射；不要手改生成的 `ti_msp_dl_config.c/.h`。

## 6. 典型功能接线

| 器件功能 | MSPM0 方向 | 建议连接 |
|---|---|---|
| CONVST | 输出 | 普通 GPIO；高速时由 Timer/Event 驱动方案决定 |
| BUSY/DRDY | 输入 | GPIO input；先轮询，再考虑边沿中断 |
| RESET | 输出 | GPIO output，保持已知上电电平 |
| CS、RD | 输出 | GPIO output 或外设片选能力 |
| SCLK | 输出 | SPI SCLK |
| SDO/DOUT | 输入 | SPI MISO；有多条数据线时逐条确认 |
| DBx | 输入 | 并行 GPIO 输入，注意需要大量 Pin |
| GND | — | MCU 与 ADC 数字地必须有正确参考关系 |

表中没有 MSPM0 封装 Pin 号，因为它取决于你的 LaunchPad/自制板、SysConfig 选脚和器件完整型号。

## 7. 并行模式怎么理解

并行模式通常用 `DB0...DB15` 一次读出一个转换字。典型思路是：等待 BUSY 完成，拉低 CS，按规定产生 RD 动作，每次采一个数据字，依次放入 `raw[channel]`，读完一帧后释放 CS。

优点是吞吐高、每个字读取快；缺点是占用 GPIO 多，MSPM0 读离散 GPIO 时还可能需要拼位。必须从具体 datasheet 确认 DB 宽度、RD/CS 时序、通道顺序以及是否有并行 byte mode。

## 8. 串行模式怎么理解

串行模式通常在转换完成后，通过 `CS + SCLK + DATA` 移出整帧结果。某些成员可能有一条或多条数据输出线。MCU 要确认：每通道多少位、是否补齐到 16/24/32 位、先发哪个通道、MSB/LSB 顺序以及 SPI 采样边沿。

优点是省 Pin、便于 SPI + DMA；缺点是总线必须在下一次采样前读完整帧。最低所需有效位率可先估算为：

```text
bus_bit_rate > sample_rate × channels × bits_per_channel
```

还要给 CS 间隔、BUSY 时间、软件开销和裕量留空间。

## 9. 最小 Bring-Up：按这个顺序做

1. 对照完整料号 datasheet 和模块原理图确认供电、基准、输入范围、数字电平。
2. 模拟输入先接 GND 或安全范围内的已知直流，所有通道都不得悬空。
3. 配置 GPIO；串行模式再配置低速 blocking SPI。
4. 按具体时序执行硬件 Reset，并等待规定时间。
5. 把 OS、RANGE、接口模式固定到一个已知配置。
6. 软件只触发一次 CONVST。
7. 有超时地等待 BUSY/DRDY，不要无限死等。
8. 只读一帧全部通道，打印十六进制 raw 值。
9. 检查通道顺序、符号、零点、满量程方向和单调性。
10. 单次读取可靠后，才加入 Timer；最后才升级 DMA 和双缓冲。

## 10. Generic main 代码框架

下面是 `GENERIC TEMPLATE`，用于看懂流程，不是可直接链接的正式 Driver。所有 `TODO_MODEL_SPECIFIC_*` 都必须由具体型号 README/Driver 按官方资料替换。

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"

#define ADC_CHANNEL_COUNT  (TODO_MODEL_SPECIFIC_CHANNEL_COUNT)
#define BUSY_TIMEOUT_LOOPS (100000UL)

bool TODO_MODEL_SPECIFIC_Reset(void);
bool TODO_MODEL_SPECIFIC_SetFixedMode(void);
void TODO_MODEL_SPECIFIC_StartConversion(void);
bool TODO_MODEL_SPECIFIC_IsBusy(void);
bool TODO_MODEL_SPECIFIC_ReadChannels(int32_t *raw, size_t channel_count);

int main(void)
{
    int32_t raw[ADC_CHANNEL_COUNT] = {0};
    uint32_t timeout = BUSY_TIMEOUT_LOOPS;

    SYSCFG_DL_init();

    if (!TODO_MODEL_SPECIFIC_Reset() ||
        !TODO_MODEL_SPECIFIC_SetFixedMode()) {
        __BKPT(0); /* 模式或 Reset 失败 */
    }

    TODO_MODEL_SPECIFIC_StartConversion();

    while (TODO_MODEL_SPECIFIC_IsBusy() && (timeout > 0U)) {
        timeout--;
    }
    if (timeout == 0U) {
        __BKPT(0); /* BUSY 超时：先查接线、极性和时序 */
    }

    if (!TODO_MODEL_SPECIFIC_ReadChannels(raw, ADC_CHANNEL_COUNT)) {
        __BKPT(0); /* 整帧读取失败 */
    }

    /* 在调试器/UART 中检查 raw[0] ... raw[channel_count - 1]。 */
    while (1) {
        __WFI();
    }
}
```

## 11. 高速连续采样怎么升级

按顺序升级，每一步都保留可回退的验证点：

```text
软件 CONVST + BUSY 轮询 + 单帧读取
→ Timer 固定 Fs，CPU 仍读取
→ BUSY 边沿中断，CPU 仍读取
→ SPI/数据通路 DMA，单 Buffer
→ Ping-Pong Buffer，处理与采集并行
→ 加 overrun 计数、帧序号和带宽监控
```

不要一开始同时上 Timer、Event、DMA、双缓冲，否则错一根线或一个边沿时很难定位。并行 GPIO 是否能直接 DMA 取决于具体 MSPM0 外设通路和引脚布局，不能仅凭“并口”二字假定可行。

## 12. 比赛时最常改什么

| 参数 | 影响 |
|---|---|
| 完整型号配置 | 通道数、位宽、接口和解析方法 |
| `sample_rate_hz` | Timer 周期、数据带宽、模拟抗混叠要求 |
| `channel_count` | 每帧数据量和 buffer 长度 |
| `range` / `vref` | raw 到电压的换算 |
| `interface_mode` | SPI 或并行 GPIO 资源 |
| `oversampling` | 带宽、延迟和噪声 |
| `busy_timeout` | 故障响应，不能代替真实时序计算 |

## 13. 常见错误

- 把某个 AD7606 并口示例直接套到 AD7606B/C 串行模块。
- 未确认模块板数字 I/O 电平就与 3.3 V MSPM0 直连。
- BUSY 极性写反，程序永远等待或过早读数。
- `raw[]` 通道顺序、符号扩展或位宽解析错误。
- Timer 周期只看 ADC 标称速率，没有为整帧读出留带宽。
- 模拟输入悬空或超出所选 RANGE，误以为数字接口坏了。

## 14. 如果比赛给我另一个同类型号怎么办

可以复用的是：`CONVST → BUSY/DRDY → 读完整帧 → 保存 raw[]` 的思路，以及“先单次、后 Timer、最后 DMA”的 Bring-Up 顺序。

必须重新查的是：完整 Pinout、通道数、分辨率、最大采样率、模拟范围、数字 I/O 电平、CONVST 数量/边沿、BUSY 极性、OS/RANGE 用法、串并行模式、数据码型、每帧位数与通道顺序。把这些填入一个新的具体型号目录，不要修改本 Family 教程来伪装成通用 Driver。

## 15. 官方资料与下一步

- [AD7606 产品页](https://www.analog.com/en/products/ad7606.html)
- [AD7606/AD7606-6/AD7606-4 官方 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ad7606_7606-6_7606-4.pdf)
- [EVAL-AD7606 评估板与 UG-851](https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/EVAL-AD7606.html)
- [AD7606B 产品页](https://www.analog.com/en/products/AD7606B.html)
- [AD7606C 评估板、User Guide、no-OS Driver 入口](https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/eval-ad7606c-18.html)
- [并行 ADC 拼装 Recipe](../../00_docs/recipes/PARALLEL_ADC_RECIPE.md)
- [SPI 连续型 ADC 拼装 Recipe](../../00_docs/recipes/SPI_STREAMING_ADC_RECIPE.md)

拿到完整型号后，从官方 Datasheet 依次读：Pin Functions、Power Supplies/Reference、Analog Input、Conversion Control、Digital Interface、Timing、Data Coding、Reset/Power-Up。`DATASHEET_REQUIRED` 的含义是这些型号相关事实必须核对，不是 README 可以省略教程。
