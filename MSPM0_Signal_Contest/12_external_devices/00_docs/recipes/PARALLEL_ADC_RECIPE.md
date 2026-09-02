# 并行 ADC 接入 Recipe

适用于 AD7606 系列及类似带 D0…Dn 并行数据总线、CONVST、BUSY、RD/CS 的外置 ADC。并行接口占用 GPIO 多，但读取直观、吞吐率高。

## 1. 先画信号分组

- 数据：`D0..D15`（或实际位宽）；
- 转换控制：`CONVST`；
- 状态：`BUSY`；
- 读取控制：`CS`、`RD`；
- 模式：`PAR/SER`、`OS0..OS2`、量程选择；
- 电源、参考与地。

不要只看模块板丝印。先确认芯片型号、模块板电平转换、数据总线电压和控制引脚有效电平。

## 2. MSPM0 SysConfig

1. 为数据总线添加 GPIO Input。若可能，优先让多位落在同一个 GPIO Port，便于一次读取。
2. 为 `BUSY` 添加 GPIO Input；第一次使用轮询，稳定后可加下降/上升沿中断。
3. 为 `CONVST`、`CS`、`RD` 添加 GPIO Output，并设置安全空闲电平。
4. 模式/量程/过采样脚如果不常变化，可硬件固定；需要比赛中修改时再设为 GPIO Output。
5. 保存后使用生成的端口和 pin mask 宏，不直接写神秘数字。

## 3. 阻塞采一帧

### 【比赛现场直接复制】

```c
static uint16_t parallel_adc_read_one(void)
{
    DL_GPIO_clearPins(GPIO_ADC_CTRL_PORT, GPIO_ADC_CONVST_PIN);
    __NOP(); __NOP(); __NOP();
    DL_GPIO_setPins(GPIO_ADC_CTRL_PORT, GPIO_ADC_CONVST_PIN);

    while ((DL_GPIO_readPins(GPIO_ADC_BUSY_PORT, GPIO_ADC_BUSY_PIN) != 0U)) {
    }

    DL_GPIO_clearPins(GPIO_ADC_CTRL_PORT, GPIO_ADC_CS_PIN |
                                          GPIO_ADC_RD_PIN);
    uint32_t port_value = DL_GPIO_readPins(GPIO_ADC_DATA_PORT,
                                           GPIO_ADC_DATA_MASK);
    DL_GPIO_setPins(GPIO_ADC_CTRL_PORT, GPIO_ADC_RD_PIN |
                                        GPIO_ADC_CS_PIN);

    return (uint16_t)((port_value & GPIO_ADC_DATA_MASK) >> DATA_SHIFT);
}
```

上面只适用于数据线连续落在一个端口的情况。若跨端口，明确写一个小型 pack 函数；不要假设逻辑位号等于 GPIO bit 位号。

## 4. 不能照抄的部分

`CONVST` 脉宽、BUSY 有效极性、RD 建立/保持时间、多通道读取次数、数据编码（二补码/偏移二进制）都由具体 ADC 决定。必须从 timing diagram 替换，不能用上面几个 `__NOP()` 当通用延时保证。

## 5. 首次验证

接固定直流电压，连续读取 100 次：应在合理码值附近小幅抖动。随后测 GND、中间电压、接近满量程三个点。逻辑分析仪至少同时观察 CONVST、BUSY、RD 和一根数据线。

高吞吐版本可用 GPIO port 读、Timer/Event 触发或 DMA，但只有在阻塞版电气和码型正确后再做。

