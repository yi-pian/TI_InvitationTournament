# Resource Conflict Guide

先把所有模块需要的 resource 写在一张表里，再进入 SysConfig。相同 peripheral instance、DMA channel、Event channel、pin 或 IRQ 只能有一个明确 owner。

## 已验证 Profile 分配

| Profile | ADC | DMA | Timer | Event | DAC / Comparator | UART |
|---|---|---|---|---|---|---|
| P01 ADC | ADC0 PA25 | CH0 ADC | TIMG0 sample | 1 | — | UART0 PA10/11 |
| P02 DualADC | ADC0 PA25 + ADC1 PA17 | CH0 A、CH1 B | TIMG0 common | 1、2 | — | UART0 PA10/11 |
| P03 DAC | — | CH1 DAC | TIMG6 DAC | 3 | DAC0 PA15 | UART0 PA10/11 |
| P04 ADC+DAC | ADC0 PA25 | CH0 ADC、CH1 DAC | TIMG0 ADC、TIMG6 DAC | 1、3 | DAC0 PA15 | UART0 PA10/11 |
| P05 Capture | — | — | TIMG6 Capture | 4 | COMP0 PA27 | UART0 PA10/11 |
| P06 Full | ADC0 PA25 + ADC1 PA17 | CH0 A、CH2 B、CH1 DAC | TIMG0 ADC、TIMG6 DAC、TIMG7 Capture | 1、2、3、4 | DAC0 PA15、COMP0 PA27 | UART0 PA10/11 |
| P08 ADC FIFO Max | ADC0 PA25 + FIFO | CH0 ADC，word/word | — | — | — | — |

## 常见冲突

| 冲突 | 如何发现 | 调整方法 |
|---|---|---|
| 两模块使用同一 DMA channel | `.syscfg` 中两个 `DMA_CHANNEL.peripheral.$assign` 相同；SysConfig solver/error | 给其中一条链分配空闲 channel，并同步检查生成宏和 Adapter 假设 |
| `adc_dma` 与 `adc_fifo_dma` 同时加入 | 两者都想拥有 ADC0、ADC0 IRQ 和 DMA0；还会出现重复 ISR | 二选一；固定/可调 Fs 用前者，最高吞吐单帧用后者 |
| ADC 与 DAC 复用同一 Timer | 两个模块都占 TIMG0/TIMG6 | 使用 P04 分配：ADC=TIMG0，DAC=TIMG6；确认 Timer 能发布对应 Event |
| Capture 与 DAC 都要 TIMG6 | 合并 P03 与 P05 时出现 | 参考 P06：保留 DAC=TIMG6，把 Capture 移到 TIMG7 |
| Event channel 重复 | publisher channel ID 相同但 receiver 不同 | P06 固定 1=ADC-A、2=ADC-B、3=DAC、4=Comparator；换号后同时改 source/receiver |
| pin 复用 | PinMux solver 或 schematic 检查 | 选择该 peripheral 支持的另一 pin；同时改 module field 和 `$assign` |
| ADC0/ADC1 选错 pin/channel | config index 与 `.syscfg` 不一致 | 以 `.syscfg` + 生成 header 为准，同时修正 config 注释/标度 |
| IRQ handler 名称/instance 不匹配 | link undefined ISR 或运行无中断 | 从生成 header 读取 IRQ 宏和 handler；不要猜名字 |
| UART0 PA10/11 被其他模块占用 | PinMux conflict 或串口消失 | 保留默认 UART；必须复用时显式换 UART pin 并接受板卡连接变化 |
| DAC DMA repeat 与其他 DMA owner 冲突 | DMA done IRQ/transfer 异常 | 使用唯一 DAC platform owner，禁止两个 wrapper 同时启动同一 channel |
| P06 资源超集无必要占用 | SysConfig 能过但可用资源少 | 真正拼装时从最小 P01~P05/P08 开始，只在确需组合时用 P06 |
| TFT SPI 与其他 SPI 设备冲突 | 两设备占同一 SPI/CS，或回调改变共享总线 mode/频率 | 共用 SCLK/MOSI 时给每个设备独立 CS，并在事务前恢复正确 mode/bitrate；不共用时换 SPI instance |
| TFT 示例引脚被复用 | PB9/PB8/PB6/PB15/PB12 已有 owner | 以 `.syscfg` 为准换到该 SPI 支持的 pin，DC/RESET/BL 可换任意安全 GPIO；同步更新回调 |
| 4×4 键盘与已有 GPIO 冲突 | 八个 row/column 中任一 pin 已被 ADC/SPI/UART/板载资源占用 | 在 SysConfig 为键盘重新选 8 个空闲数字 GPIO；模块源码不依赖固定 pin |
| TFT 刷屏阻塞键盘扫描 | 长时间整屏 SPI 写导致 5 ms 键盘任务不能按时运行 | TFT 局部刷新、分块绘制，把 Keypad Scan 放在固定周期任务；不要在中断里清屏 |
| 普通按键 pin 与板载资源重叠 | 外接按键选择了 LED、SWD、UART、VREF 或已占用 pin | 优先用板载 S2/PB21 或文档示例 PB1；更换时以 SysConfig owner 表为准 |
| 自锁开关 pin 与 PWM/其他 GPIO 重叠 | 示例 PA28 已被 PWM 或输出模块占用 | 换任意安全空闲 digital input，并同步回调；不要让两个模块同时配置同一 pin |
| 同一个按钮同时由 IRQ 和轮询拥有 | ISR 与 Update 都对同一动作计数 | 入门工程只保留周期轮询 owner；需要 IRQ 时单独设计事件边界 |

## 合并模块前检查顺序

1. 列每个模块的 ADC/DAC/COMP/OPA/GPAMP instance 和 pin。
2. 列 Timer owner、period/clock、发布的 Event channel。
3. 列 DMA channel、direction、width、repeat mode。
4. 列 Event publisher/receiver、IRQ 和 ISR owner。
5. 对照上表选择最接近的已验证 Profile。
6. SysConfig generate，处理 error；warning 单独记录。
7. full link 后再检查 `.map`，资源不冲突不等于 RAM 足够。

TFT 与矩阵键盘的具体接线和 LP-MSPM0G3507 示例 pin 表分别见：

- [`../01_bsp/tft_ili9341/README.md`](../01_bsp/tft_ili9341/README.md)
- [`../01_bsp/matrix_keypad_4x4/README.md`](../01_bsp/matrix_keypad_4x4/README.md)
- [`../01_bsp/button/README.md`](../01_bsp/button/README.md)
- [`../01_bsp/latching_button_switch/README.md`](../01_bsp/latching_button_switch/README.md)

不要通过手改生成文件“解决”冲突；下一次 generate 会覆盖，而且 source of truth 仍然错误。
