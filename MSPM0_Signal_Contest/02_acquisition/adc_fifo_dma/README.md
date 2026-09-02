# ADC FIFO DMA：MSPM0 满吞吐率单帧采集

## CCS SysConfig GUI Configuration

### Required resources

需要 `ADC12` 和其依赖的 `DMA` channel，不需要 `TIMER`/`EVENT`。P08 是满吞吐参考：`SIGNAL_ADC_FIFO`、`ADC0`、PA25、`DMA_CH0`，这些不是全局固定资源。

### Step 1 - ADC12 continuous FIFO

GUI Path: 左侧 `Add` -> `ADC12` -> 实例 -> `Basic Configuration` -> `Sampling Mode Configuration` -> `Conversion Mode/Repeat Mode`；继续展开 `ADC Conversion Memory Configurations` -> `ADC Conversion Memory 0 Configuration` -> `Input Channel`、`Device Pin Name`、`Sample Period Source`。再进入同一实例的 `Advanced Configuration` -> `Conversion Resolution`、`Power Down Mode`、`Desired Sample Time 0`、`FIFO Mode/Enable FIFO`。P08 基线对应 Channel 2、PA25、Sampling Timer 0、12-bits、Manual、62.5 ns、FIFO enabled；重复/自动转换在 `Sampling Mode Configuration` 中开启，不把它写成代码枚举。

### Step 2 - ADC functional clock

P08 `.syscfg` 的证据值为 `sampClkSrc = DL_ADC12_CLOCK_SYSOSC`；在 GUI 中从左侧 `ADC12` -> 实例 -> `Basic Configuration` -> `Clock Configuration`（部分 SDK 将它放在 `ADC12 Clock Configuration`）进入，设置/核对 `Clock Source`、`Clock Divider` 和右侧计算出的 ADC functional clock。该时钟不是采样率；吞吐由 functional clock、resolution、sample time 和 FIFO result rate 共同决定，没有 Timer 提供任意精确 Fs。

### Step 3 - DMA Configuration

GUI Path: 左侧 `ADC12` -> 实例 -> `DMA Configuration` -> `Configure/Enable DMA Trigger`；在 `DMA Trigger Source` 选择对应 conversion-memory result（P08 为 MEM10 result loaded），`DMA Samples Count` 填 6。随后进入左侧 `DMA` -> 该 channel -> `Address Mode` -> `Fixed address to Block address`，并在 `Transfer Configuration` -> `Source Length`、`Destination Length` 选择 WORD/WORD。P08 生成 `SIGNAL_ADC_FIFO_DMA_CHAN_ID = 0` 仅作基线，实际 channel 必须选择当前空闲资源。

### PinMux / Event

在 `PinMux Peripheral and Pin Configuration` 核对 ADC instance 与 analog pin。本模式不添加 `EVENT`，也不创建 Timer publisher。

### Expected generated symbols

核对 `SIGNAL_ADC_FIFO_INST`、`SIGNAL_ADC_FIFO_ADCMEM_0`、`SIGNAL_ADC_FIFO_DMA_CHAN_ID`、`SIGNAL_ADC_FIFO_INST_DMA_TRIGGER` 和 GPIO/IOMUX analog pin 宏。PROJECT_AUDIT 记录 `GUI field -> .syscfg property -> generated symbol`。

### Final checklist / Common mistakes / Do not change

- FIFO、连续转换、DMA request threshold 与 buffer 宽度相互匹配。
- 不把 P08 的 SYSOSC 枚举当成已确认 GUI field；不声称得到任意精确 `Fs`。
- 不直接编辑 `.syscfg` 或生成文件，不照搬 PA25/DMA_CH0。

以上路径已经覆盖本模块需要的 GUI 字段；保存并点击 SysConfig 的 Generate 后，再在 `ti_msp_dl_config.h` 核对实例、FIFO、DMA trigger 和 channel 宏。

## 0. 30 秒选型

当题目需要在很短时间里尽可能密地采一帧，例如观察运放上升沿、下降沿或做高采样率频谱时，选本模块。

当题目明确要求一个可调而且严格的采样率，例如 100 kSPS、500 kSPS，选旁边的 `adc_dma`。两者不是新旧关系，而是用途不同：

| 模块 | ADC 什么时候采下一点 | 优点 | 代价 |
|---|---|---|---|
| `adc_dma` | Timer 经 Event 每来一次触发采一点 | Fs 可设置，时间轴清楚 | 触发链和逐点 DMA 限制最高吞吐 |
| `adc_fifo_dma` | 上一次转换结束后 ADC 自动开始下一次 | 不占 Timer/Event，吞吐更高 | Fs 由 ADC 时钟、分辨率和 sample time 决定，不能运行时任意设置 |

## 1. 它解决什么问题

普通逐点 DMA 每得到一个 ADC 结果就发一次 DMA 请求。高速时，请求、仲裁和搬运本身会成为额外负担。

本模块启用 ADC FIFO：

```text
PA25 模拟电压
  -> ADC0 自动连续转换
  -> FIFO：两个 12-bit 结果打包成一个 32-bit word
  -> DMA_CH0：一次搬 32 bit
  -> uint16_t raw[N]
```

这样 DMA 请求次数约减半，而且没有 Timer/Event 逐样点触发开销。应用仍然得到按时间顺序排列的 `uint16_t` 数组，不需要自己拆包。

本实现基于 MSPM0 SDK 2.11.00.07 的 TI 官方 `driverlib/adc12_max_freq_dma` 例程，并增加了正式 API、参数检查、状态查询和可重复 Start 所需的 FIFO 清理流程。

## 2. 比赛时复制哪些文件

从本目录复制到应用工程的同一源码目录：

1. `signal_adc_fifo_dma.c`
2. `signal_adc_fifo_dma.h`
3. `README_MINIMAL_EXAMPLE.c` 只用来查 main 写法，不要和自己的 main 同时加入 Build

再复制公共头文件：

4. `01_bsp/common/signal_status.h`

SysConfig 最省事的方式是参考：

5. `09_examples/integration_profiles/PROFILE_08_ADC_FIFO_MAX/profile.syscfg`

应用 Include Path 至少要能找到这些模块文件、`signal_status.h` 和生成的 `ti_msp_dl_config.h`。

## 3. SysConfig / Pin

### 3.1 最快方法

母应用还没有占用 ADC0/PA25/DMA_CH0 时，打开 `PROFILE_08_ADC_FIFO_MAX/profile.syscfg`，在 SysConfig 中逐项照着配置。不要把生成的 `ti_msp_dl_config.c/.h` 从别的工程硬复制过来；每个应用都应由自己的 `.syscfg` 重新生成。

### 3.2 手动配置顺序

在 CCS 中打开应用自己的 `.syscfg`：

1. 添加一个 **ADC12** instance，名称必须改成 `SIGNAL_ADC_FIFO`。
2. 物理 ADC 选择 `ADC0`。
3. MEM0 输入选择 `ADC channel 2`，引脚选择 `PA25`。
4. 分辨率选择 `12-bit`，数据格式选择 `unsigned`，参考源选择 `VDDA`。
5. ADC clock 选择 `SYSOSC`，divider 选择 `/1`，频率范围选择 `24 to 32 MHz`。
6. sampling source 选择 `AUTO`，trigger source 选择 `SOFTWARE`，repeat mode 打开。
7. MEM0 的 trigger mode 选择 `AUTO_NEXT`。
8. sample timer 选择 `SCOMP0`，Sample Time 0 设为 `62.5 ns`（生成代码通常是 2 个 ADC clock cycle）。
9. Power Down Mode 选择 `MANUAL`。
10. 打开 **FIFO**。
11. 打开 **Configure DMA**，DMA instance 名称必须改成 `SIGNAL_ADC_FIFO_DMA`，通道可选空闲的 DMA channel；本 profile 使用 `DMA_CH0`。
12. DMA address mode 选择 `fixed to block`，source/destination width 必须是 `WORD / WORD`。
13. ADC DMA samples count 设为 `6`，DMA trigger 选择 `MEM10 result loaded`。
14. ADC interrupt 只勾选 `DMA done`。
15. 生成后确认 `ti_msp_dl_config.h` 里存在 `SIGNAL_ADC_FIFO_INST`、`SIGNAL_ADC_FIFO_DMA_CHAN_ID` 和 `SYSCFG_DL_SIGNAL_ADC_FIFO_init()`。

这里没有 Timer，也没有 Event route。因为 ADC 不是“等外部节拍再采”，而是软件启动后自动连续转换。

### 3.3 换 ADC 引脚时改什么

只在 SysConfig 改 MEM0 channel 和对应 analog pin，例如从 ADC0.2/PA25 换到题目实际接线。不要只改 pin 不改 channel，也不要只在 C 代码里写一个通道数字；真实模拟复用由 SysConfig 生成。

### 3.4 资源冲突

本模块独占一个 ADC instance、它的 IRQ 和一个 DMA channel。它不能与使用同一个 ADC0 IRQ 的 `adc_dma` 同时加入同一工程，也不能让其他模块占用相同 DMA channel。

## 4. 最小调用

完整可编译版本在 `README_MINIMAL_EXAMPLE.c`。核心顺序只有四步：

```c
#define SIGNAL_SAMPLE_COUNT (1024U)
_Alignas(4) static uint16_t g_raw[SIGNAL_SAMPLE_COUNT];

const signal_adc_fifo_dma_config_t config = { 4000000U };

SYSCFG_DL_init();
SignalADCFIFODMA_Init(&config);
SignalADCFIFODMA_Start(g_raw, SIGNAL_SAMPLE_COUNT);
while (!SignalADCFIFODMA_IsFinished()) { __WFI(); }
```

### 为什么是 `_Alignas(4)`

DMA 每次写 32 bit，所以数组起始地址必须是 4 的倍数。数组元素虽然是 `uint16_t`，但 `_Alignas(4)` 明确要求编译器把整个数组放在 4-byte aligned 地址。

### 为什么 N 必须是偶数

FIFO 的一个 32-bit word 内含两个 12-bit 样本，DMA transfer size 是 `N / 2` 个 word。奇数 N 会剩半个 word，本模块直接返回 `SIGNAL_RESULT_INVALID_ARGUMENT`，避免悄悄少一个样本。

### 为什么应用不需要拆 FIFO

MSPM0G3507 是 little-endian，DMA 把 FIFO word 写进 `uint16_t[]` 后，两半字自然成为相邻元素：

```text
FIFO word 0 -> raw[0], raw[1]
FIFO word 1 -> raw[2], raw[3]
...
```

所以后面的 ADC To Voltage、Vpp、RMS、FFT 等仍直接读取 `raw[0..N-1]`。

## 5. 每个函数是干什么的

### `SignalADCFIFODMA_Init(&config)`

- 输入：一个非零 `nominal_sample_rate_hz`。
- 作用：记录时间轴要使用的名义 Fs，清空模块状态，打开 ADC DMA-done IRQ。
- 不做的事：它不会根据这个数字改 ADC 寄存器。真正速率来自 SysConfig。

### `SignalADCFIFODMA_Start(buffer, N)`

- 检查已经 Init、当前不忙、buffer 4-byte aligned、N 为非零偶数。
- 复位 ADC 并重新调用生成的 `SYSCFG_DL_SIGNAL_ADC_FIFO_init()`，清掉上一帧可能残留的 FIFO 数据。
- DMA source 指向 ADC FIFO，destination 指向 buffer，transfer size 设置为 `N/2` 个 32-bit word。
- 打开 DMA/ADC 后用 software trigger 启动连续转换。

为什么每次都 reset/re-init：DMA 完成后 CPU 进入中断需要几个时钟，ADC 可能已经又转换了少量样本并放进 FIFO。如果直接重启，下一帧开头可能混入上一帧尾部。ADC reset 是清空 FIFO 的可靠边界，生成的 instance init 再恢复正确配置。

### `SignalADCFIFODMA_IsFinished()`

DMA 已经把整帧搬完后返回 true。等待时可以 `__WFI()`，让 CPU 睡眠到中断，而不是空转。

### `SignalADCFIFODMA_Stop()`

中途终止采集，关闭 conversions、ADC DMA 和 DMA channel，并回到 `MODULE_IDLE`。它不清空用户数组；下一次 Start 会 reset ADC。

### 查询函数

- `SignalADCFIFODMA_GetStatus()`：当前 IDLE/RUNNING/DONE/ERROR。
- `SignalADCFIFODMA_GetBuffer()`：最近一次 Start 的 buffer。
- `SignalADCFIFODMA_GetSampleCount()`：最近一次 N。
- `SignalADCFIFODMA_GetNominalSampleRateHz()`：Init 记录的名义 Fs。
- `SignalADCFIFODMA_GetModuleMaturity()`：当前证据等级。

## 6. 采样率到底是多少

“满速”不是一个对所有配置都固定的数字。它表示 ADC 完成一次转换后立刻开始下一次。实际 conversion period 由这些配置共同决定：

- ADC clock source 与 divider；
- 8/10/12-bit resolution；
- Sample Time 0；
- 芯片和模拟输入是否满足数据手册条件。

本 profile 在当前 SDK/SysConfig 中显示约 250 ns conversion period，因此 main 示例记录：

```text
Fs_nominal = 1 / 250 ns = 4,000,000 sample/s
```

如果你修改了 ADC clock、分辨率或 sample time，必须同时修改应用传给 config 的 `nominal_sample_rate_hz`。这个字段只用于后续计算：

```text
第 i 点时间 t[i] = i / Fs
两点时间差    dt   = (i2 - i1) / Fs
FFT bin 频率       = bin * Fs / N
```

它不是示波器实测值。对压摆率等对时间轴敏感的测量，应再用已知频率信号或示波器校验真实 Fs，必要时把校准后的值作为 nominal Fs 传入。

## 7. 最常修改的参数

| 参数 | 去哪里改 | 当前值 | 为什么改 | 影响 |
|---|---|---:|---|---|
| ADC channel / pin | SysConfig 的 MEM0 | ADC0.2 / PA25 | 与实际接线一致 | 接错会读悬空或其他信号 |
| N | 应用 `SIGNAL_SAMPLE_COUNT` | 1024 | 覆盖足够时间或提高 FFT 分辨率 | RAM=`2N` byte，采集时间=`N/Fs` |
| nominal Fs | 应用 config | 4,000,000 Hz | 与 SysConfig conversion period/实测校准一致 | 全部时间、频率、SR 结果按比例变化 |
| sample time | SysConfig | 62.5 ns | 高源阻抗时给采样电容更多建立时间 | 建立更准但 Fs 下降 |
| ADC clock/divider | SysConfig | SYSOSC / 1 | 改吞吐率或满足数据手册 | conversion period 和 nominal Fs 都变 |
| ADC reference | SysConfig + 换算参数 | VDDA | 与硬件参考一致 | raw-to-voltage 比例改变 |
| DMA channel | SysConfig | DMA_CH0 | 避开其他模块占用 | C 代码不用改，生成宏会更新 |

## 8. 与算法模块怎么连接

本模块输出仍是标准 ADC raw：

```text
adc_fifo_dma -> ADC To Voltage Recipe -> Vpp/RMS/AC RMS/Remove DC/FFT
```

连接时确认：

- 输入/输出类型：本模块是 `uint16_t raw[N]`，电压算法需要 raw、N、VREF 和 full-scale code。
- 样本顺序：FIFO 已由 DMA 展开成正常顺序，不需要 Adapter。
- 单位：raw 没有 V 单位；必须转换后才能与 1.65 V、3 Vpp 等题目条件比较。
- Fs：FFT、过零时间和压摆率必须使用与这帧相同的 nominal/校准 Fs。

## 9. 常见错误

### 编译提示找不到 `SIGNAL_ADC_FIFO_INST`

SysConfig instance 名字不是 `SIGNAL_ADC_FIFO`，或工程没有重新 Generate。改 instance name 后重新生成。

### 编译提示找不到 `SYSCFG_DL_SIGNAL_ADC_FIFO_init`

同样是 instance 命名不一致。该函数由 SysConfig 根据名字生成，不是本模块自己声明的。

### Start 返回 INVALID_ARGUMENT

检查 N 是否为非零偶数，以及数组是否用 `_Alignas(4)` 声明。

### 结果时间比示波器短或长

不要先改阈值算法。先确认代码使用的 Fs 是否等于当前 SysConfig conversion period 对应值；时间结果与 Fs 成反比。

### raw 波形幅值偏小或高频失真

满速 ADC 仍需要模拟输入在 acquisition time 内给内部采样电容充到正确电压。检查信号是否在 0~VDDA、调理电路带宽、运放驱动能力和源阻抗；必要时增加 sample time，此时也要降低 nominal Fs。

### 测 2 MHz 正弦却幅值不稳定

4 MSPS 对 2 MHz 只有约 2 点/周期，采样相位稍变就可能错过峰值，而且已到 Nyquist 边界。满速不等于任何 0~2 MHz 波形都能可靠测幅；需要更多 points/cycle、包络/RMS 方法或更快的外部 ADC。

## 10. 验证状态与边界

- SysConfig generation：PASS。
- TI Arm Clang 5.1.1.LTS，`-Wall -Werror` compile：PASS。
- 隔离空工程 full link：PASS。
- 实板波形、真实 Fs、重复 Start 数据边界：`NOT_RUN`。
- 成熟度：`MODULE_STATUS_BUILD_VERIFIED`，不能写成 BOARD_VERIFIED。

第一次上板至少检查：固定 DC raw 是否稳定、已知正弦测得 samples/cycle 是否符合 Fs、连续重复采集的首点是否异常、最大输入不超过 ADC 量程。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“adc_fifo_dma”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalADCFIFODMA_Init -> SignalADCFIFODMA_Start -> SignalADCFIFODMA_IsFinished -> SignalADCFIFODMA_GetStatus -> SignalADCFIFODMA_GetBuffer -> SignalADCFIFODMA_GetSampleCount -> SignalADCFIFODMA_GetNominalSampleRateHz -> SignalADCFIFODMA_GetModuleMaturity -> SignalADCFIFODMA_Stop
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块需要 SysConfig。先在 CCS 的 .syscfg 添加并核对：ADC12、DMA；再按前文的模块专用 GUI 步骤选择实际 pin/instance。保存后让 CCS 重新生成配置，核对生成宏；不要直接修改 	i_msp_dl_config.c/.h，也不要照抄示例 pin 或 DMA/Event 编号。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_result_t SignalADCFIFODMA_Init(const signal_adc_fifo_dma_config_t *config);`

**它做什么：** 初始化 ADC FIFO DMA 模块状态。

**什么时候调用：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `config` | `const signal_adc_fifo_dma_config_t *` | 名义采样率元数据，不能为 0。 |

**返回：** SIGNAL_RESULT_OK 表示成功。

**最小调用形状：** `SignalADCFIFODMA_Init(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalADCFIFODMA_Start(uint16_t *buffer, uint16_t sample_count);`

**它做什么：** 以 ADC 可连续完成转换的最高吞吐方式采集一帧。

**什么时候调用：** 启动一轮新的硬件操作或异步传输；成功后按对应的完成查询 API 等待结果。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `buffer` | `uint16_t *` | DMA 目标缓冲区，必须 4 字节对齐。 |
| `sample_count` | `uint16_t` | uint16_t 样本数，必须为非零偶数。 |

**返回：** SIGNAL_RESULT_OK 表示采集已经启动。

**最小调用形状：** `SignalADCFIFODMA_Start(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `void SignalADCFIFODMA_Stop();`

**它做什么：** 立即停止采集并把模块恢复到空闲状态。

**什么时候调用：** 主动终止当前操作并释放模块占用的运行状态；只在需要取消本轮任务时调用。

**参数：** 无。

**返回：** 无返回值；调用后按本函数的后置状态或后续查询 API 判断效果。

**最小调用形状：** `SignalADCFIFODMA_Stop(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `bool SignalADCFIFODMA_IsFinished();`

**它做什么：** 完整一帧已经搬入 RAM 时返回 true。

**什么时候调用：** 查询一个布尔条件，例如一帧数据是否已准备好；它不会等待也不会处理数据。

**参数：** 无。

**返回：** 返回 `true` 或 `false`；它只表示本次查询条件是否成立。

**最小调用形状：** `SignalADCFIFODMA_IsFinished(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_status_t SignalADCFIFODMA_GetStatus();`

**它做什么：** 返回 IDLE、RUNNING、DONE 或 ERROR。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**参数：** 无。

**返回：** 返回 `signal_status_t` 类型的值；成功后再使用输出数据，失败时不要消费输出。

**最小调用形状：** `SignalADCFIFODMA_GetStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `const uint16_t * SignalADCFIFODMA_GetBuffer();`

**它做什么：** 返回最近一次 Start 使用的缓冲区；未启动过时为空指针。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**参数：** 无。

**返回：** 返回 `const uint16_t *` 类型的值；成功后再使用输出数据，失败时不要消费输出。

**最小调用形状：** `SignalADCFIFODMA_GetBuffer(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `uint16_t SignalADCFIFODMA_GetSampleCount();`

**它做什么：** 返回最近一次 Start 的样本数；未启动过时为 0。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**参数：** 无。

**返回：** 返回 `uint16_t` 类型的值；成功后再使用输出数据，失败时不要消费输出。

**最小调用形状：** `SignalADCFIFODMA_GetSampleCount(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `uint32_t SignalADCFIFODMA_GetNominalSampleRateHz();`

**它做什么：** 返回 Init 记录的 SysConfig 名义采样率，单位 Hz。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**参数：** 无。

**返回：** 返回 `uint32_t` 类型的值；成功后再使用输出数据，失败时不要消费输出。

**最小调用形状：** `SignalADCFIFODMA_GetNominalSampleRateHz(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalADCFIFODMA_GetModuleMaturity();`

**它做什么：** 返回模块证据成熟度。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 当前实现中出现的返回/成熟度枚举值：`MODULE_STATUS_BUILD_VERIFIED`。

**最小调用形状：** `SignalADCFIFODMA_GetModuleMaturity(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

