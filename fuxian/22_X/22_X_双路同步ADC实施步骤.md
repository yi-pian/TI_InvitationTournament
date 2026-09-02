# 22_X 双路同步 ADC 实施步骤

## 1. 模块选择

补充说明：本工程的双 ADC 模块副本已同步到集成库当前正式版本。DMA 通道完成中断现在由 `SignalDualADC_Init()` 内部自动打开，main 不再需要另写 `DL_DMA_enableInterrupt()`。22_X 使用 DMA_CH0/CH1，模块会根据 SysConfig 生成的通道宏自动选择这两路。

题目要求是用两路同步 ADC 采集 X、Y 两个外部输入，频率范围 1.5 kHz 至 10 kHz。因此选择 `MSPM0_Signal_Contest/02_acquisition/adc_dual_sync/` 的正式硬件入口 `signal_dual_adc_mspm0g3507.c/.h`。

未选择同目录 `signal_adc_dual_sync.c/.h`，因为它只对已有交织数组做拆分，不能采集 ADC。未选择单 ADC DMA，因为该题要求两路由同一触发事件同步采样。

## 2. 比赛时复制的模块文件

按模块 README 的“30 秒接入”复制到 `signal_contest_template_final/modules/`：

| 文件 | 来源 |
|---|---|
| `signal_dual_adc_mspm0g3507.c` | `02_acquisition/adc_dual_sync/` |
| `signal_dual_adc_mspm0g3507.h` | `02_acquisition/adc_dual_sync/` |
| `signal_status.h` | `01_bsp/common/` |

三个复制文件均保持原样，未编辑其 `.c/.h` 内容。

## 3. SysConfig 配置

配置文件：`signal_contest_template_final/signal_contest_template.syscfg`。

按模块 README 第 3 节和 `PROFILE_02_DUAL_ADC/profile.syscfg` 的已验证资源配置：

| 功能 | 实例与配置 |
|---|---|
| X 输入 | `SIGNAL_ADC_A`，ADC0，Memory 0 Input Channel 2，PA25 |
| Y 输入 | `SIGNAL_ADC_B`，ADC1，Memory 0 Input Channel 2，PA17 |
| 两路 ADC | Event trigger、Repeat Mode、Memory 0 result loaded DMA trigger、DMA done interrupt |
| X DMA | `SIGNAL_ADC_A_DMA`，DMA_CH0，Half Word，Fixed-to-Block，Single |
| Y DMA | `SIGNAL_ADC_B_DMA`，DMA_CH1，Half Word，Fixed-to-Block，Single |
| 公共节拍 | `SIGNAL_DUAL_ADC_TIMER`，TIMG0，BUSCLK，Periodic，10 us 初值 |
| 同步事件 | TIMG0 ZERO_EVENT 发布到 Event 1/2；ADC A/B 分别订阅 1/2 |

与 README 的不同：无功能性不同。README 把 PA25/PA17 声明为可按比赛接线替换的参考 Pin；题目未指定接线，因此采用 `PROFILE_02_DUAL_ADC` 已验证的默认映射。参考 profile 内的 UART0 未加入，因为本题不要求串口，避免占用额外资源。

## 4. main.c 代码来源

`main.c` 由模块 README 的 `README_MINIMAL_EXAMPLE.c` 直接复制，包含：

1. `signal_dual_adc_mspm0g3507.h`。
2. 每路 100 kSPS、每帧 1024 点的 A/B 缓冲区。
3. `SYSCFG_DL_init()` 后调用 `SignalDualADC_Init()`。
4. 每帧调用 `SignalDualADC_Start(g_raw_a, g_raw_b, SIGNAL_SAMPLE_COUNT)`。
5. 通过 `while (!SignalDualADC_IsFinished()) { __WFI(); }` 等待两路 DMA 完成。

`g_raw_a[i]` 与 `g_raw_b[i]` 是同一次 TIMG0 触发下的 X/Y 原始 ADC 码。100 kSPS 对 10 kHz 最高输入频率提供每周期约 10 点；1024 点采样窗为约 10.24 ms，可覆盖 1.5 kHz 输入约 15 个周期。

## 5. 自编写内容

仅做以下比赛工程组合工作，未写模块驱动或算法：

1. 将母版 `.syscfg` 按 README 配置为双 ADC、双 DMA、公共 Timer/Event。
2. 将 `signal_config.h` 的预期信号范围改为 1.5 kHz 至 10 kHz；`main.c` 采用 README 自带的 `100000U`/`1024U` 参数。
3. 填写 `COPIED_MODULES.md` 和本文档。

没有修改任何复制到 `modules/` 的 `.c/.h` 文件；`main.c` 也没有加入 README 之外的业务逻辑。

## 6. 比赛现场操作顺序

1. 确认 X/Y 模拟前端输出均在 MSPM0 ADC 允许范围内。
2. 在 CCS 打开 `.syscfg`，按上表核对 ADC0/PA25、ADC1/PA17、DMA_CH0/1、TIMG0 和 Event 1/2；若实际接线不同，只替换两个合法 ADC Pin/Channel，保持其他同步链路不变。
3. 保存并 Generate，核对生成头文件中有 `SIGNAL_ADC_A`、`SIGNAL_ADC_B`、两路 `DMA_CHAN_ID` 与 `SIGNAL_DUAL_ADC_TIMER` 宏；禁止手改生成文件。
4. 在 CCS Refresh 工程，确认 `modules/signal_dual_adc_mspm0g3507.c` 没有 Exclude from Build。
5. Build。下载前最后确认模拟输入幅度、X/Y 接线和公共地。
