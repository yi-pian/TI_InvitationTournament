# TI DriverLib 比赛现场速查（MSPM0G3507）

> 已按 MSPM0 SDK `2.11.00.07` 本机头文件核对。这里是“找到入口”的速查表，不代替函数上方 Doxygen、SysConfig、datasheet/TRM。第一次使用或不理解参数时，回到 [TI_DRIVERLIB_BEGINNER_GUIDE.md](TI_DRIVERLIB_BEGINNER_GUIDE.md)。

## 先做这三件事

```c
#include "ti_msp_dl_config.h"

int main(void)
{
    SYSCFG_DL_init();                 /* 先完成生成的静态初始化 */
    /* 再调用运行时 DriverLib API */
}
```

1. 打开自己工程生成的 `ti_msp_dl_config.h`，使用其中的实例、port、pin、MEM、channel、IRQ 宏。
2. 如果仓库已有 `ADC_DMA`、`DAC_DMA`、Timer Capture 等正式模块，优先用模块，不在 `main.c` 重写底层链路。
3. `SYSCFG_DL_init()` 已配置的 clock/PinMux/power/reset/init 不要重复做。

## 一页常用动作

| 我要做什么 | 当前 SDK 真实 API | 最容易错的点 |
|---|---|---|
| 短粗略延时 | `DL_Common_delayCycles(cycles)` | 参数是 CPU cycle；`0` 是最大延时，不是 0 延时 |
| GPIO 置高 | `DL_GPIO_setPins(port, pinMask)` | pin 是位掩码 |
| GPIO 置低 | `DL_GPIO_clearPins(port, pinMask)` | active-low 设备置低通常是 asserted |
| GPIO 翻转 | `DL_GPIO_togglePins(port, pinMask)` | 不用于设置已知初值 |
| 读 GPIO | `DL_GPIO_readPins(port, pinMask)` | 用 `(result & pinMask) != 0U` 判断 |
| 打开输出驱动 | `DL_GPIO_enableOutput(port, pinMask)` | 不能代替 PINCM/PinMux 配置 |
| 开 GPIO 中断 | `DL_GPIO_enableInterrupt(port, pinMask)` | 还需 `NVIC_EnableIRQ(...)` |
| 查 GPIO 待处理中断 | `DL_GPIO_getPendingInterrupt(port)` | 返回 IIDX，不是 pin mask |
| 清 GPIO 中断 | `DL_GPIO_clearInterruptStatus(port, pinMask)` | 参数是 pin mask，不是 IIDX |
| 启动 Timer | `DL_TimerG_startCounter(timer)` | Timer 参数应已由 SysConfig 配好 |
| 停止 Timer | `DL_TimerG_stopCounter(timer)` | 停止不会清空配置 |
| 改/读 LOAD | `DL_TimerG_setLoadValue(timer, value)` / `getLoadValue` | 频率还受时钟、divider、prescaler、模式影响 |
| 读当前计数 | `DL_TimerG_getTimerCount(timer)` | 注意计数方向和回绕 |
| 读 Capture | `DL_Timer_getCaptureCompareValue(timer, ccIndex)` | `ccIndex` 是 CC 通道，不是 GPIO |
| 启用 ADC 转换 | `DL_ADC12_enableConversions(adc)` | 通常是对配置好的 ADC 重新武装 |
| 启动 ADC | `DL_ADC12_startConversion(adc)` | 触发模式由 SysConfig 决定 |
| 读 ADC result | `DL_ADC12_getMemResult(adc, memIdx)` | 返回 raw code，不是 volt；memIdx 不是物理通道 |
| 停 ADC | `DL_ADC12_stopConversion(adc)` | 块采集优先正式 `ADC_DMA` |
| 写 12-bit DAC | `DL_DAC12_output12(dac, code)` | code `0..4095`；Vref/输出范围另查 |
| 开/关 DAC | `DL_DAC12_enable(dac)` / `disable` | 连续波形优先 `DAC_DMA` |
| 配 DMA 地址 | `DL_DMA_setSrcAddr` / `setDestAddr` | 传地址值；宽度、增量要匹配 |
| 配 DMA 次数 | `DL_DMA_setTransferSize(dma, ch, size)` | 单位是 transfer，不一定是 byte |
| 使能 DMA channel | `DL_DMA_enableChannel(dma, ch)` | 外设触发通常不是 `startTransfer` |
| 软件触发 DMA | `DL_DMA_startTransfer(dma, ch)` | 只适合 software trigger |
| SPI 阻塞发 8 bit | `DL_SPI_transmitDataBlocking8(spi, data)` | 会等到 SPI 不 busy |
| SPI 阻塞取 RX | `DL_SPI_receiveDataBlocking8(spi)` | controller 必须先发送来产生 SCLK |
| SPI 非阻塞尝试发 | `DL_SPI_transmitDataCheck8(spi, data)` | `false` 表示 FIFO 满，未发送 |
| SPI 非阻塞尝试收 | `DL_SPI_receiveDataCheck8(spi, &byte)` | `false` 表示 RX FIFO 空 |
| SPI 批量填 FIFO | `DL_SPI_fillTXFIFO8(spi, buf, count)` | 返回实际填入数，不保证全发完 |
| SPI 批量读 FIFO | `DL_SPI_drainRXFIFO8(spi, buf, maxCount)` | 只读当前已有数据 |
| SPI 是否仍发送 | `DL_SPI_isBusy(spi)` | 不是 RX available |
| I2C 预填 TX FIFO | `DL_I2C_fillControllerTXFIFO(i2c, buf, count)` | 检查实际填入数 |
| I2C 发起一次 burst | `DL_I2C_startControllerTransfer(i2c, addr, dir, len)` | 自动 START+STOP；void 不代表成功 |
| I2C repeated-start | `DL_I2C_startControllerTransferAdvanced(...)` | START/STOP/ACK 枚举按官方例程，不猜 |
| I2C 看状态 | `DL_I2C_getControllerStatus(i2c)` | 区分 `BUSY`、`BUSY_BUS`、`ERROR` |
| I2C 取一个 byte | `DL_I2C_receiveControllerData(i2c)` | 先确认 RX FIFO 非空 |
| UART 阻塞发 byte | `DL_UART_Main_transmitDataBlocking(uart, byte)` | 可能阻塞，实时链慎用 |
| UART 阻塞收 byte | `DL_UART_Main_receiveDataBlocking(uart)` | 没数据会一直等 |
| UART 非阻塞尝试发 | `DL_UART_Main_transmitDataCheck(uart, byte)` | `false` 表示 FIFO 满 |
| UART 非阻塞尝试收 | `DL_UART_Main_receiveDataCheck(uart, &byte)` | `false` 表示 FIFO 空 |
| UART 填/排 FIFO | `DL_UART_Main_fillTXFIFO` / `drainRXFIFO` | 返回实际 byte 数 |
| UART TX 是否忙 | `DL_UART_Main_isBusy(uart)` | 不表示 RX 是否有数据 |
| 比较器读输出 | `DL_COMP_getComparatorOutput(comp)` | 返回枚举，模拟路由先由 SysConfig 配 |
| 调比较器 DAC0 | `DL_COMP_setDACCode0(comp, code)` | code 不是 volt，要看参考和模式 |
| 读复位原因 | `DL_SYSCTL_getResetCause()` | 启动诊断使用 |
| ISR 后进入 sleep | `DL_SYSCTL_enableSleepOnExit()` | 主循环逻辑可能因此不再执行 |
| 等 MathACL 完成 | `DL_MathACL_waitForOperation(MATHACL)` | 先正确配置 Q 格式/符号/operation |

## 参数识别速查

| 参数长相 | 实际意思 | 去哪里取 |
|---|---|---|
| `GPIO_Regs *gpio` | GPIOA/GPIOB 实例 | 生成的 `..._PORT` 宏 |
| `SPI_Regs *spi`、`ADC12_Regs *adc` | 外设寄存器实例 | 生成的 `..._INST` 宏 |
| `uint32_t pins` | 一个或多个 pin 的位掩码 | 生成的 `..._PIN` 宏；可用 `|` |
| `uint32_t pincmIndex` | IOMUX PINCM 索引 | 生成的 `..._IOMUX` 宏；不是 pin mask |
| `DL_ADC12_MEM_IDX idx` | ADC conversion memory 槽 | 生成的 `..._ADCMEM_...` 宏 |
| `uint8_t channelNum` | DMA channel 编号 | SysConfig channel ID 宏 |
| `interruptMask` | 外设 interrupt 位掩码 | 对应 `dl_xxx.h` 的 `DL_xxx_INTERRUPT_*` |
| `DL_xxx_IIDX` | 最高优先级 pending source 索引 | `getPendingInterrupt()` 返回；ISR 中 switch |
| `count/length/maxCount` | 数量 | 必须读 Doxygen，可能是 byte、word 或 transfer |

## 五个最小片段

### GPIO 控 CS

```c
DL_GPIO_clearPins(GPIO_DEVICE_CS_PORT, GPIO_DEVICE_CS_PIN);
/* SPI transaction */
DL_GPIO_setPins(GPIO_DEVICE_CS_PORT, GPIO_DEVICE_CS_PIN);
```

### SPI 写两个 byte

```c
DL_GPIO_clearPins(GPIO_DEVICE_CS_PORT, GPIO_DEVICE_CS_PIN);
DL_SPI_transmitDataBlocking8(SPI_0_INST, command);
DL_SPI_transmitDataBlocking8(SPI_0_INST, value);
while (DL_SPI_isBusy(SPI_0_INST)) {}
DL_GPIO_setPins(GPIO_DEVICE_CS_PORT, GPIO_DEVICE_CS_PIN);
```

### I2C 写两个 byte

```c
uint8_t tx[2] = {reg, value};
uint16_t loaded = DL_I2C_fillControllerTXFIFO(I2C_0_INST, tx, 2U);
if (loaded == 2U) {
    DL_I2C_startControllerTransfer(I2C_0_INST, address7,
        DL_I2C_CONTROLLER_DIRECTION_TX, 2U);
}
```

### ADC 单个 result

```c
DL_ADC12_startConversion(ADC12_0_INST);
/* 等待 configured interrupt/status */
uint16_t raw = DL_ADC12_getMemResult(
    ADC12_0_INST, ADC12_0_ADCMEM_0);
```

### Timer 周期运行

```c
NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
DL_TimerG_startCounter(TIMER_0_INST);
```

## 中断固定模型

```c
SYSCFG_DL_init();
DL_xxx_enableInterrupt(INST, DL_xxx_INTERRUPT_SOURCE);
NVIC_ClearPendingIRQ(INST_INT_IRQN);
NVIC_EnableIRQ(INST_INT_IRQN);

void INST_IRQHandler(void)
{
    switch (DL_xxx_getPendingInterrupt(INST)) {
        case DL_xxx_IIDX_SOURCE:
            /* 快速搬数据或置 flag */
            break;
        default:
            break;
    }
}
```

上面是结构模板，`DL_xxx...` 不是可直接编译的函数名。必须从当前外设头文件/生成宏换成真实名字；`IIDX` 与 interrupt mask 不能互换。

## SysConfig 已经做什么

| 看到的 API | 默认判断 |
|---|---|
| `enablePower`、`reset`、`setClockConfig`、`init...`、PinMux | 多半在 `ti_msp_dl_config.c`，应用不要重复 |
| `setPins/clearPins/readPins` | 应用运行时操作 |
| `startCounter/stopCounter/getTimerCount` | 应用运行时操作 |
| `startConversion/getMemResult` | 单次调试可直接用；N 点块采集走模块 |
| `transmit/receive/fill/drain/isBusy` | 通信运行时操作 |
| `setPublisherChanID/setSubscriberChanID` | 默认由 SysConfig 维护 Event 路由 |

## 现场排错顺序

1. 生成的 `ti_msp_dl_config.h` 中有实例/引脚宏吗？
2. `SYSCFG_DL_init()` 调了吗？
3. PinMux、方向、上拉、SPI mode、I2C 地址、UART baud 与 datasheet/接线一致吗？
4. API 参数是 instance、pin mask、PINCM、MEM idx、IIDX 还是 interrupt mask？
5. blocking 调用卡在哪里？有 clock、ACK、RX data 或外设 ready 吗？
6. FIFO/DMA 返回的实际数量是否被忽略？
7. 中断是否同时启用了外设 mask 和 NVIC IRQ？
8. Event publisher/subscriber 是否在 SysConfig 两端都连接？

## 陌生函数 60 秒查法

```text
看 DL_ 后的外设前缀
→ 在 C:\ti\mspm0_sdk_2_11_00_07\source\ti\driverlib 找 dl_xxx.h
→ 搜完整函数名
→ 向上读 Doxygen 的参数/范围/前置条件
→ 或打开 docs\chinese\driverlib\mspm0g1x0x_g3x0x_api_guide\html\index.html
→ 追 enum/mask/struct 定义
→ 在 LP_MSPM0G3507/driverlib examples 搜真实调用
→ 检查 ti_msp_dl_config.c 是否已初始化
```

常用官方示例关键字：`gpio_toggle_output`、`timg_32bit_timer_mode_periodic_sleep`、`timx_timer_mode_capture`、`adc12_single_conversion`、`adc12_max_freq_dma`、`dac12_dma_sampletimegen`、`spi_controller_multibyte_fifo_poll`、`i2c_controller_rw_multibyte_fifo_poll`、`i2c_controller_rw_repeated_start_fifo_interrupts`、`uart_rw_multibyte_fifo_poll`。
