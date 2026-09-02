# 从 Datasheet 到驱动：用 AD9850 完整走一遍

这个案例故意不从最终代码开始。我们只使用 Analog Devices 官方 [AD9850 Rev. H Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ad9850.pdf) 建立结论，再用本地旧工程确认项目曾采用过哪种控制方式。

## 1. 先写最小目标

目标不是“实现所有 DDS 功能”，而是：

```text
MSPM0 四根 GPIO
→ Reset 并进入 AD9850 serial mode
→ 写 1 kHz
→ 示波器看到约 1 kHz 连续输出
```

最小可观察结果明确后，才知道代码写到哪里算完成。

## 2. 从哪里找供电和电平

在 Datasheet 首面的 Features 找到 3.3 V/5 V single-supply；再看 Specifications，而不是只看首页：

- 5 V 供电最高参考时钟 125 MHz；3.3 V 条件为 110 MHz。
- CMOS logic input 在 5 V 供电时 `VIH(min)=3.5 V`，在 3.3 V 时为 2.4 V。
- 因此“MSPM0 是 3.3 V”不能直接推出“任意 5 V AD9850 板都能保证识别”。
- Absolute Maximum 页还警告器件上电前不要施加数字输入。

软件结论：驱动不能解决电平不兼容；README 必须把电平转换与上电顺序放在代码前。

## 3. 怎样读 Pinout

Datasheet Pin Function 表告诉我们串行控制的四根线：

- Pin 7 `W_CLK`；
- Pin 8 `FQ_UD`；
- Pin 25 `D7` 同时是 serial data；
- Pin 22 `RESET`。

这一步也抓出了本地旧 8051 源码注释中的错误：它把 RESET 写成 PIN12，而官方 Pin 12 是 `RSET`。结论不是“旧代码都没用”，而是：**旧代码能提示控制流程，官方 Datasheet 才决定接线。**

模块板排针仍要再看板子资料，因为板子可能只引出这些信号，且顺序不同。

## 4. 怎样从 Timing 图提取协议

Programming 章节和 Table IV 给出：

1. 串行数据共 40 bit。
2. W0 是 FTW 的 LSB，W31 是 FTW 的 MSB。
3. W32/W33 必须为 0，W34 是 power-down，W35..W39 是 5-bit phase。
4. 每个 W_CLK 上升沿移入一 bit。
5. 40 bit 后给 FQ_UD 脉冲提交。
6. Reset 后用一个 W_CLK 脉冲再一个 FQ_UD 脉冲进入 serial load；裸 IC 还要满足 Figure 11 的绑定。

Timing Characteristics 再给最小值：数据建立/保持、W_CLK 高低脉宽、FQ_UD 高低脉宽和 Reset 周期。第一次 Bring-Up 用 1 µs 边沿等待，远慢于纳秒最小值，便于看波形，也不追求极限更新速度。

## 5. 把频率公式变成不会丢精度的代码

Datasheet 给 32-bit FTW：

```text
FTW = round(fout × 2^32 / fref)
```

不能先做 32-bit 乘法，也不需要用 `double`。用 64-bit 中间量：

```c
numerator = ((uint64_t) frequency_hz << 32U) +
            (reference_clock_hz / 2U);
ftw = (uint32_t) (numerator / reference_clock_hz);
```

125 MHz 参考、1 MHz 输出的独立已知结果是 `0x020C49BA`。这成为 PC 单元测试的锚点。

## 6. 先画 API 边界，再调用 DriverLib

如果核心驱动直接保存 `GPIO_Regs *`，PC 就难以验证 bit 顺序，换平台也会重写。因此只抽象两件事：

```c
bool write_line(context, W_CLK/FQ_UD/DATA/RESET, high);
void delay_us(context, time_us);
```

核心 `ad9850.c` 负责 FTW、40-bit 帧、Reset 和状态；`ad9850_mspm0_platform.c` 才调用 `DL_GPIO_setPins/clearPins` 和 `DL_Common_delayCycles`。这是两层，不是大型框架。

## 7. SysConfig 从 Datasheet 得到什么

Datasheet 说“需要四根数字输入”，所以 SysConfig 只需四个 GPIO output；它不要求 SPI 外设，因为协议是 40-bit LSB-first + 独立 FQ_UD，GPIO Blocking 最容易第一次看清。

配置：

1. GPIO group `DDS_GPIO`；
2. `W_CLK/FQ_UD/DATA/RESET` 四根输出；
3. 初始低；
4. 选择无资源冲突的 Pin；
5. Generate 后把生成的 PORT/PIN 宏填进 platform context。

若模块晚于 MCU 上电，要先保持 Pin 高阻。SysConfig 具体操作参见 [`../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md`](../../00_docs/SYSCONFIG_BEGINNER_GUIDE.md)。

## 8. Init 顺序怎样落代码

```text
四线低
→ RESET 高至少 5 个参考时钟，再拉低
→ 等至少 2 个参考时钟恢复
→ W_CLK 脉冲
→ FQ_UD 脉冲
→ 发送 40-bit 全零帧并 FQ_UD 提交
```

为什么额外发全零：官方说明 Reset 不影响 data input register；显式写完整帧可把软件依赖收口到已知值。不能只“写一个频率字段”而保留未知控制位。

## 9. 最小测试怎样设计

PC mock 在每个 W_CLK 上升沿记录 DATA：

- `AD9850_Init` 后清记录；
- 设置 1 MHz、phase code=3；
- 断言正好记录 40 bit；
- 前 32 bit LSB-first 还原为 `0x020C49BA`；
- 后 8 bit 是 `0x18`（phase 3 左移 3，W32/W33/W34 为 0）；
- 超过 `fref/2` 返回 OUT_OF_RANGE。

这个测试证明“算式、位序、控制位”正确，不证明电源、晶振、输出滤波或板子工作。因此状态只能到 `PC_VERIFIED`。

## 10. 上板验证如何闭环

1. 限流上电，确认电流和温升正常。
2. 逻辑分析仪数 40 个 W_CLK，人工核对第一个字节 LSB-first。
3. 看 FQ_UD 只在完整帧之后提交。
4. 示波器测 1 kHz、10 kHz、100 kHz。
5. 如果全按固定比例偏差，核对晶振；如果只高频变差，查滤波、镜像和负载。
6. 断电重启三次，并保存接线、波形、模块板号，才升级 `BOARD_VERIFIED`。

## 11. 这个方法怎样迁移到别的器件

把 AD9850 换成任何陌生芯片，顺序不变：

```text
明确最小结果
→ 官方工作条件/电平
→ Pin 分类
→ Timing 翻译
→ Reset/寄存器默认值
→ 最小 API 边界
→ SysConfig
→ 纯计算/协议 PC 测试
→ Blocking 上板
→ 最后才加 DMA/综合应用
```

区别只是：SPI ADC 的核心层会打包寄存器并解析 raw，OLED 核心层会管理命令/显存，数字电位器会编码抽头地址。不要跳过电气和最小验证阶段。

