# SysConfig Modify Guide

`.syscfg` 是 pinmux、peripheral instance、Timer、DMA、Event、IRQ 和生成初始化的唯一配置源。不要手改生成的 `ti_msp_dl_config.c/h`。

## 先选最近的 Profile

| 需要的硬件组合 | Profile | 当前资源 |
|---|---|---|
| 单 ADC DMA | `PROFILE_01_ADC_CAPTURE` | ADC0/PA25、DMA0、TIMG0、Event1、UART0 |
| Dual ADC | `PROFILE_02_DUAL_ADC` | ADC0/PA25 DMA0 + ADC1/PA17 DMA1、TIMG0、Event1/2 |
| DAC DMA | `PROFILE_03_DAC_GENERATOR` | DAC0/PA15、DMA1、TIMG6、Event3 |
| ADC + DAC | `PROFILE_04_ADC_DAC` | ADC chain 同 P01；DAC chain 同 P03 |
| Comparator Capture | `PROFILE_05_FREQUENCY` | COMP0/PA27、Event4、TIMG6 Capture |
| DualADC + DAC + Capture | `PROFILE_06_FULL_SIGNAL` | DMA0 ADC-A、DMA2 ADC-B、DMA1 DAC；TIMG0/TIMG6/TIMG7；Event1~4 |
| 单 ADC FIFO 满吞吐率 | `PROFILE_08_ADC_FIFO_MAX` | ADC0/PA25、FIFO、32-bit DMA0；无 Timer/Event |

路径：`09_examples/integration_profiles/<PROFILE>/profile.syscfg`。

## 常见修改

### 更换 ADC channel / pin

1. 找应用所用的 `SIGNAL_ADC`、`SIGNAL_ADC_FIFO`、`SIGNAL_ADC_A` 或 `SIGNAL_ADC_B` instance。
2. 同时修改 `adcMem0chansel` 和对应 `adcPinX.$assign`；不能只改 config 中的 channel 数字。
3. 确认该 pin 没被 UART/DAC/Comparator/OPA 占用。
4. generate 后打开生成 `ti_msp_dl_config.h`，确认 instance/pin 宏和 `SYSCFG_DL_init()` 仍一致。

### 更换 sample rate

1. 找 `SIGNAL_SAMPLE_TIMER` 或 `SIGNAL_DUAL_ADC_TIMER`。
2. 确认应用是运行时根据 Fs 重配 Timer，还是直接使用 SysConfig `timerPeriod`。
3. 若改 `timerPeriod`，保留 timer clock source/divider/prescale 的一致性。
4. generate 后运行应用时读取 `SignalADC_GetConfiguredTriggerRate()`，不要只相信期望值。

若使用 `adc_fifo_dma`，不要执行上面四步：它没有采样 Timer。打开 P08 的 `SIGNAL_ADC_FIFO`，由 ADC clock/divider、resolution 和 Sample Time 共同决定 conversion period，再令应用的 nominal Fs=`1/conversion period`。改完必须同步修改 `signal_adc_fifo_dma_config_t.nominal_sample_rate_hz`，并用已知信号校验实际时间轴。

### 增加第二路 ADC

1. 从 `PROFILE_02_DUAL_ADC` 或 P06 开始，不要在 P01 旁边随便复制 instance。
2. 选择不同 ADC instance、pin 和 DMA channel。
3. 两路使用同一个 Timer 的不同 Event publisher channel。
4. 应用使用 `signal_dual_adc_platform.*` 的两块独立 buffer，不建立 interleaved 假设。

### 增加/更换 DMA

1. 在 ADC/DAC instance 的 `DMA_CHANNEL.peripheral.$assign` 查看 channel。
2. 对照 `RESOURCE_CONFLICT_GUIDE.md`，确保没有另一个 owner。
3. 普通 `adc_dma` 使用 peripheral-to-block、half-word、single；`adc_fifo_dma` 使用 peripheral-to-block、**word/word**，因为一个 FIFO word 含两个样本；DAC 使用 block-to-peripheral、half-word、repeat single。不要只换 channel 而改变方向/宽度。
4. generate 后检查生成 DMA instance 宏，并 full link。

### 使用 DAC

1. 参考 P03/P04/P06 的 DAC12、`SIGNAL_DAC_TIMER`、Event3 和 DMA1。
2. 检查 DAC output pin、FIFO、hardware trigger、DMA direction 和 Timer period。
3. 应用通过 `signal_dac_dma_platform.*` 启停，不直接把 DriverLib 搬进 `main.c`。

### 使用 Comparator Capture

1. 参考 P05：`SIGNAL_COMP` + `SIGNAL_CAPTURE`。
2. 检查 comparator input pin、reference、hysteresis/filter、Event publisher channel。
3. Capture Timer 的输入 event、clock、period/overflow 和 IRQ 都必须匹配。
4. 当前 P05 Timer 向下计数；应用 Adapter 负责转换为正向 timestamp。

### 更改 UART / PinMux

1. 当前 profile 使用 UART0 PA10/PA11，115200。
2. 改 pin 时同时检查板卡默认 UART 连接；PA10/PA11 被改作其他功能会失去默认串口。
3. 重新 generate，并确认没有与 ADC/DAC/Comparator pin 冲突。

### 增加 ILI9341 SPI 彩屏

1. 先读 [`../01_bsp/tft_ili9341/README.md`](../01_bsp/tft_ili9341/README.md) 的电气与接线警告。
2. 添加 SPI Controller：mode 0、8 bit、MSB first；首次先用 1 MHz。
3. 配 DC 输出；RESET/BL 按实际屏板决定。CS 只能在“SPI 硬件 CS”与“GPIO 手动 CS”中选一种。
4. LP-MSPM0G3507 的现成 profile 在 `09_examples/tft_ili9341_lp_mspm0g3507/tft_ili9341.syscfg`，真实分配为 PB9/PB8/PB6/PB15/PB12。
5. projectspec 只链接 `01_bsp/tft_ili9341/signal_tft_ili9341.c`，不要把驱动复制进应用。

### 增加 4×4 矩阵键盘

1. 添加 4 个 Digital Output，初值全部为高；添加 4 个 Digital Input，开启 internal pull-up。
2. 先确认键盘排针的 R1..R4/C1..C4 顺序；无丝印时用万用表，不能猜。
3. 平台回调把 `active row` 映射为输出低，把 `column pin low` 映射为 pressed。
4. 推荐应用每 5 ms 调一次 Scan，`settle_us=5`、`debounce_scans=3`。
5. 完整 LP-MSPM0G3507 八脚建议与接线表见 [`../01_bsp/matrix_keypad_4x4/README.md`](../01_bsp/matrix_keypad_4x4/README.md)。

### 增加普通瞬时按键

1. 配一个 Digital GPIO Input 并启用 internal pull-up；按键另一端接 GND。
2. 平台回调把 pin low 转换为 `pressed=true`。
3. 推荐每 5 ms 调一次 `SignalButton_Update()`，`debounce_scans=3`。
4. LP-MSPM0G3507 可直接使用板载 S2/PB21；外接示例可用 PB1（40-pin 第 39 脚）。
5. 四脚轻触按钮先用万用表确认两侧，不要把内部永久相连的同侧两脚当开关。

### 增加自锁按键开关

1. 三脚开关先用万用表确认 COM/NO/NC；推荐 COM→GND、NO→pull-up GPIO input。
2. 平台回调把 pin low 转换为 `on=true`；若使用 NC，只反转回调极性。
3. 等待 `state_valid=true` 后使用 `stable_on`；不要对物理转换再做软件 toggle。
4. LP-MSPM0G3507 示例使用 PA28（40-pin 第 38 脚），已有 PWM owner 时必须换 pin。
5. 发光按钮的 LED 端子单独按电压、电流和限流要求处理，不属于开关输入 SysConfig。

## 修改后必须检查

1. 保留 `.syscfg` 的 device/package/product/version metadata。
2. 运行 SysConfig CLI；warning 单独记录，不能称为 clean。
3. 检查生成 `ti_msp_dl_config.h` 的 instance、IRQ、DMA、pin 和 init 函数拼写。
4. 运行全部 translation units compile + final link，而不是只做语法检查。
5. 检查 `.map` 与 `tiarmsize`、资源冲突和 SRAM 余量。

不修改 `.syscfg` 的场景：只改软件 N、VREF 数值、频率搜索范围、窗口、阈值或 DDS 数学参数，且 peripheral instance/pin/Timer/DMA/Event 路由没有变化。
