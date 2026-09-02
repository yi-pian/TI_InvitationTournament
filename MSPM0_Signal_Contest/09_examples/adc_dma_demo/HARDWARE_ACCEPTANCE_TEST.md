# ADC_DMA LEVEL 2：动态模拟输入验收

本文件保留给未来 `FULL HARDWARE VERIFIED`。只有开发板、USB 和 CCS 时，请先执行 [BOARD-ONLY LEVEL 1](../adc_dma_onboard_selftest/BOARD_ONLY_ACCEPTANCE_TEST.md)；以下信号源/示波器步骤当前不作为阻塞项。

## 1. 验收边界

本次只证明以下链路可靠、可重复、可参数化：

```text
PA25 / ADC0.2
  -> TIMG0 ZERO_EVENT
  -> Event channel 1
  -> ADC0 MEM0
  -> DMA_CH0
  -> uint16_t RAM buffer[N]
```

输入不经过板载 OPA2365，不改 ADC 通道。本文的 raw min/max/mean 只用于验收，不代表已经建立测量算法模块。

## 2. 接线与仪器

```text
信号发生器 OUT  -> PA25 / J1.2 / ADC0.2
信号发生器 GND  -> LaunchPad GND
示波器 CH1      -> PA25 / J1.2
示波器 CH2      -> PA12 / J4.34 / TIMG0_CCP0
示波器 GND      -> LaunchPad GND
```

输入必须始终位于 0~VDDA；正弦测试使用 Hi-Z 幅度口径。PA12 是 DEBUG/VALIDATION 输出，`signal_adc_dma.c` 不引用它。

PA25/J1.2 与 PA12/J4.34 的板卡位置可对照 TI 官方 [LP-MSPM0G3507 User's Guide（Rev. D）](https://www.ti.com/lit/ug/slau873d/slau873d.pdf) 的 BoosterPack connector pinout。

## 3. PA12 输出代表什么

MSPM0 Event channel 1 的订阅者额度已由 ADC0 使用，不能再把同一 Event 直接订阅给 GPIO。Demo 因而把 **产生该 Event 的同一 TIMG0** 的 CCP0 输出到 PA12，并配置为每次 ZERO_EVENT 硬件翻转：

- 无逐点 CPU 中断；
- 与 ADC Event 共用同一个 Timer 计数器和 ZERO_EVENT；
- 相邻上/下边沿间隔 = `1 / configured_trigger_rate`；
- 完整方波周期 = 两个采样周期，方波频率 = 配置触发率 / 2；
- 它验证 Timer/Event 源节拍，不直接测量 ADC 采样孔径时刻，也不能单独排除 ADC/DMA 丢样。

| 配置触发率 | 相邻边沿间隔 | 完整方波周期 | CH2 方波频率 |
|---:|---:|---:|---:|
| 100 kSPS | 10 us | 20 us | 50 kHz |
| 200 kSPS | 5 us | 10 us | 100 kHz |
| 500 kSPS | 2 us | 4 us | 250 kHz |

每帧结束后 Timer 停止，统计期间 PA12 会保持电平，因此示波器上看到的是事件脉冲串。请在脉冲串内部测相邻边沿，不要把帧间空隙计入周期。

如不需要输出，把 `SIGNAL_VALIDATION_TRIGGER_OUTPUT_ENABLE` 改为 0；正式项目可直接删除 SysConfig 中的 `VALIDATION_TRIGGER_GPIO`，ADC_DMA 模块无需修改。

## 4. 采样率 API 判定

```c
SignalADC_GetConfiguredTriggerRate();
```

它只返回：

```text
timer_count = round(timer_clock_hz / requested_rate)
configured_trigger_rate = timer_clock_hz / timer_count
```

当前 BUSCLK/divider/prescaler 为 32 MHz/1/1：

| 请求值 | timer_count | LOAD | 配置触发率 |
|---:|---:|---:|---:|
| 100 kSPS | 320 | 319 | 100000 Hz |
| 200 kSPS | 160 | 159 | 200000 Hz |
| 500 kSPS | 64 | 63 | 500000 Hz |

该返回值不是外部实测采样率，不补偿实际时钟误差、Event 传播、ADC 采样孔径或潜在丢触发。验收必须同时使用 PA12 示波器周期和 RAM 中已知输入波形的 samples/cycle。

## 5. 100 次重复启动验收

Demo 初始化一次模块，然后执行：

```text
填充 0xFFFF 哨兵
SignalADC_Start()
等待 Done
检查状态/指针/点数/整帧 12-bit 数据
重复 100 次
```

每次 `Start` 都会重新停止 Timer，复位 Timer count，关闭 ADC conversion/ADC DMA/DMA channel，清 ADC DMA_DONE、DMA 原始完成位和 NVIC pending，重新写 DMA source/destination/transfer size，再按 DMA -> ADC DMA -> ADC conversion -> Timer 的顺序启动。

DMA 使用 `SINGLE` 模式：第 N 点后硬件自动关闭通道，避免 ADC ISR 到来前的下一触发覆盖 `buffer[0]`。下一帧由 `Start` 显式重新使能。

| 重启检查项 | 当前处理 |
|---|---|
| Timer | 先 stop，再把 counter 写回当前 LOAD，最后启动 |
| Event | SysConfig 初始化时使能一次并保持；空闲期 Timer 停止、ADC conversion 关闭，不产生采样 |
| ADC conversion | Start 前关闭，DMA 准备好后重新使能 |
| ADC DMA request | Start 前关闭，DMA channel 准备好后重新使能 |
| DMA source | 每帧重写为 ADC MEM0 result address |
| DMA destination | 每帧重写为本次 `buffer[0]` |
| DMA transfer size | 每帧重写为本次 `sample_count` |
| DMA enable | Start 前关闭，地址/长度完成后重新使能 |
| ADC interrupt flag | 清 `DL_ADC12_INTERRUPT_DMA_DONE` |
| DMA raw flag | 按 SysConfig 通道号推导掩码并清除 |
| NVIC pending | 清 ADC IRQ pending，IRQ 在 Init 中使能 |
| module state | Start 前拒绝 RUNNING；启动置 RUNNING；ISR 完成置 DONE |

第 100 帧断点处必须同时满足：

```text
g_acceptance_complete == true
g_acceptance_pass == true
g_completed_blocks == 100
g_validation_failure == VALIDATION_FAILURE_NONE
g_last_result == SIGNAL_RESULT_OK
g_last_module_status == MODULE_DONE
g_failed_block == UINT32_MAX
```

任何 `g_adc_buffer[i] == 0xFFFF` 都说明 DMA 没有覆盖完整缓冲区，Demo 会以 `VALIDATION_FAILURE_BUFFER_NOT_FILLED` 停止。

## 6. N 与 Fs 参数矩阵

只修改 `signal_config.h` 或编译器 `-D`，不改 `signal_adc_dma.c`：

```c
#define SIGNAL_SAMPLE_RATE_HZ (200000U)
#define SIGNAL_SAMPLE_COUNT   (2048U)
```

理论 `samples_per_cycle = Fs / Fin`；帧内周期数 `cycles_in_frame = N / samples_per_cycle`。

| Fs | 1 kHz samples/cycle | 10 kHz samples/cycle |
|---:|---:|---:|
| 100 kSPS | 100 | 10 |
| 200 kSPS | 200 | 20 |
| 500 kSPS | 500 | 50 |

| N | 1 kHz@100k 周期数 | 1 kHz@200k 周期数 | 1 kHz@500k 周期数 | 10 kHz@100k 周期数 | 10 kHz@200k 周期数 | 10 kHz@500k 周期数 |
|---:|---:|---:|---:|---:|---:|---:|
| 256 | 2.56 | 1.28 | 0.512 | 25.6 | 12.8 | 5.12 |
| 512 | 5.12 | 2.56 | 1.024 | 51.2 | 25.6 | 10.24 |
| 1024 | 10.24 | 5.12 | 2.048 | 102.4 | 51.2 | 20.48 |
| 2048 | 20.48 | 10.24 | 4.096 | 204.8 | 102.4 | 40.96 |
| 4096 | 40.96 | 20.48 | 8.192 | 409.6 | 204.8 | 81.92 |

编译准入要求覆盖 5 个 N × 3 个 Fs 共 15 组；实板至少完成下一节的四项必测，并建议逐步跑完矩阵。

### 本机编译链接记录

历史上曾用 SysConfig 1.26.2 / TI Arm Clang 4.0.2.LTS 完成下列 15 组链接；该结果只作历史记录，不再作为当前环境准入证据。2026-08-07 工程收口已用 MSPM0 SDK 2.11.00.07、SysConfig 1.28.0、TI Arm Clang 5.1.1.LTS、`-O2 -Wall -Werror` 重新验证默认配置完整链接。若正式采用扩展 N/Fs 组合，需用当前工具链重新跑对应行。

| N | 100 kSPS | 200 kSPS | 500 kSPS | 固件 RAM（含 512 B stack） |
|---:|:---:|:---:|:---:|---:|
| 256 | PASS | PASS | PASS | 1069 B |
| 512 | PASS | PASS | PASS | 1581 B |
| 1024 | PASS | PASS | PASS | 2605 B |
| 2048 | PASS | PASS | PASS | 4653 B |
| 4096 | PASS | PASS | PASS | 8749 B |

这只能证明源码、生成配置、链接和 SRAM 容量可接受；不能替代 15 组实板采集。

## 7. 四项必测

### 测试 1：默认正弦与 100 帧

- 输入：1 kHz、1.0 Vpp、1.65 V Offset、Hi-Z。
- 配置：Fs=100 kSPS，N=1024，100 blocks。
- 预期：约 100 samples/cycle，约 10.24 cycles/frame。
- 理想原始码：mean≈2048，min≈1427，max≈2668。
- 示波器 PA12：相邻边沿约 10 us。
- 通过：100 帧判据全部成立，无 0xFFFF 哨兵残留，RAM 波形连续。

### 测试 2：200 kSPS

- 输入：1 kHz、1.0 Vpp、1.65 V Offset、Hi-Z。
- 配置：Fs=200 kSPS；N 可保持 1024。
- 预期：约 200 samples/cycle；1024 点约 5.12 周期。
- 示波器 PA12：相邻边沿约 5 us。

### 测试 3：500 kSPS

- 输入：10 kHz、建议保持 1.0 Vpp、1.65 V Offset、Hi-Z。
- 配置：Fs=500 kSPS；N 可保持 1024。
- 预期：约 50 samples/cycle；1024 点约 20.48 周期。
- 示波器 PA12：相邻边沿约 2 us。

### 测试 4：直流中点

- 输入：1.65 V DC，确认无负压、不过 VDDA。
- 配置：Fs=100 kSPS，N=1024。
- 预期：mean 约 2048，buffer 接近恒定 2048；允许真实 VDDA、源误差和 ADC 噪声造成偏差。
- 注意：本项 `min≈max≈mean` 是正确现象，必须结合已知 DC 输入判断，不能单凭“恒定 2048”认定故障。

## 8. 故障排查表

| 现象 | 可能原因 | 检查方法 | 优先排查位置 |
|---|---|---|---|
| Buffer 全 0 | PA25 接地/信号源未开；ADC 通道错误；DMA 未写 | 示波器先量 PA25；看 `g_completed_blocks`；检查 SysConfig CH2/PA25 | 1 接线与信号源；2 ADC PinMux；3 DMA dest/size |
| Buffer 全 4095 | 输入超过 VDDA；偏置/幅度设置错误；PA25 被拉高 | 示波器量 PA25 最大值；确认 Hi-Z 幅度口径 | 1 输入安全；2 信号源 Offset/Load；3 共地 |
| Buffer 恒定 2048 | 若输入是 1.65 V DC 则正常；若输入正弦则信号源未接、输出关闭或看错 buffer | CH1 直接量 PA25；Memory Browser 用 16-bit 显示并查看完整 N 点 | 1 PA25 实际波形；2 CCS 地址/格式；3 ADC 通道 |
| 只有第一个 sample 变化 | DMA destination 未递增；transfer size 为 1；其余点是旧数据 | 查生成配置 `destIncrement`、`DL_DMA_setTransferSize`；看 0xFFFF 哨兵 | 1 SysConfig DMA addressMode；2 Start 重装长度 |
| 波形周期错误 | Fs 配置/Timer 计数时钟错误；信号源频率错误；把 PA12 完整方波周期误当采样周期 | 量 CH1 输入频率；量 PA12 相邻边沿；看 `g_configured_trigger_rate_hz` | 1 仪器频率；2 BUSCLK/divider/prescaler；3 LOAD |
| 每隔固定点出现异常 | DMA 宽度/地址增量错误；输入源振铃/数字串扰；帧越界；采样保持驱动不足 | 找异常间隔是否为 2/4/缓存边界；同时观察 PA25；核对 half-word | 1 DMA width/increment；2 缓冲边界；3 模拟接线/探头地 |
| 第二次 Start 不工作 | DMA channel/ADC DMA 未重使能；完成标志或 NVIC pending 未清；状态仍 RUNNING | 看失败帧 `g_failed_block` 与 `g_validation_failure`；单步第二次 Start | 1 Start 的 disable/clear/reload/enable 顺序；2 ADC ISR 状态 |
| DMA 不结束 | Timer 没启动；Event channel 不匹配；ADC DMA trigger 错；N 被破坏 | PA12 是否有边沿；看状态是否一直 RUNNING；检查 Event.dot 和 MEM0 trigger | 1 TIMG0；2 Event publisher/subscriber=1；3 ADC MEM0/DMA trigger |
| `__WFE` 不唤醒 | ADC DMA_DONE interrupt 未使能；NVIC IRQ 未使能；ISR 名称不匹配；DMA 未完成 | 暂停 CPU，看 `g_completed_blocks`、ADC RIS/IIDX、NVIC；确认 `SIGNAL_ADC_INST_IRQHandler` | 1 ADC interrupt 配置；2 NVIC；3 DMA 完成条件 |
| 改变 N 后异常 | 数组仍是旧大小；CCS 仍按旧长度显示；N=0/超 65535；SRAM 越界 | 看 `sizeof(g_adc_buffer)`/map；Clean Project 后重编；检查宏只定义一次 | 1 `signal_config.h`；2 Clean/Rebuild；3 map/SRAM |

## 9. 验收记录

| 项目 | 实测值/结论 |
|---|---|
| 板卡序列号 |  |
| SDK / SysConfig / TI Clang | 2.11.00.07 / 1.28.0 / 5.1.1.LTS |
| 信号源 / 示波器型号 |  |
| 测试 1 |  |
| 测试 2 |  |
| 测试 3 |  |
| 测试 4 |  |
| 100 帧首次失败 block | 无 / 具体编号 |
| N/Fs 扩展矩阵 |  |
| 最终准入 | PASS / FAIL |

只有四项必测和 100 帧重复启动均通过，才能把 ADC_DMA 标为实板准入；通过前不得进入下一模块。
