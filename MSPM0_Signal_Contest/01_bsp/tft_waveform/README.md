# TFT Waveform：ILI9341 波形绘制辅助

状态：`BUILD_VERIFIED`；Board：`NOT_RUN`。它依赖现有 [TFT ILI9341 正式驱动](../tft_ili9341/README.md)，不复制 SPI、字库或基础绘图实现。

## CCS SysConfig GUI Configuration

### Required resources

完全复用 TFT ILI9341 的一个 `SPI` + 控制 `GPIO`，本 waveform helper 不新增 Timer、DMA、Event、IRQ 或 Pin。`SPI` 是 SysConfig module，验证 example 的 `SPI1` 是硬件 instance，`SPI_TFT` 是工程实例名，`DL_SPI_*` 是 DriverLib C 名称。

### Step 1 - Configure SPI controller and PinMux

GUI Path: 左侧 `Add` -> `SPI` -> 实例 `SPI_TFT` -> `Basic Configuration` -> `Controller/Peripheral Mode`、`Word Length`、`Clock Polarity`、`Clock Phase`；然后展开 `PinMux Peripheral and Pin Configuration` 设置 SCLK/PICO/CS0。SPI1/PB9/PB8/PB6 只是验证 profile 的基线，当前接线优先。

### Step 2 - Configure SPI clock

GUI Path: 左侧 `SPI` -> `SPI_TFT` -> `Basic Configuration` -> `Clock Configuration`（或 `SPI Controller Advanced Configuration`）-> `Functional Clock Source`、`Clock Divider/Prescaler`、`Baud Rate/SCLK Frequency`。核对 `system/BUS clock -> SPI functional clock -> baud divider -> SCLK`，不要把 CPUCLK_FREQ 当成 SCLK。

### Step 3 - Configure control GPIO

GUI Path: `Add` -> `GPIO` -> `GPIO_TFT_CTRL` -> `Group Pins` / `PinMux Peripheral and Pin Configuration`。Action: verified profile 创建 `TFT_DC = PB15` output、`TFT_BLK = PB12` output 且 Initial Value 为 SET；按当前硬件保留或调整。普通 GPIO 时钟状态为 `NO_INDEPENDENT_PERIPHERAL_CLOCK`。

### Step 4 - Confirm no extra timing resources

Action: 本模块的 UI 刷新节拍由 Application 调度，不创建采样 Timer；波形采样率由上游 ADC 链决定。若应用另有 ADC/Timer/DMA，它们属于采集模块，不能记到 `tft_waveform` 合同下。

### Expected generated symbols

Generate 后核对 `SPI_TFT_INST`、`GPIO_SPI_TFT_*` 的 SCLK/PICO/CS0 宏、`GPIO_TFT_CTRL_PORT`、`GPIO_TFT_CTRL_TFT_DC_PIN`、`GPIO_TFT_CTRL_TFT_BLK_PIN` 和 `CPUCLK_FREQ`。PROJECT_AUDIT 对照 GUI、`.syscfg` property 与当前生成 symbol，不编辑生成头文件。

保存后点击 Generate，并核对 SPI 与 TFT 控制 GPIO 的生成宏；本 waveform helper 不新增 Timer/DMA/Event。

### Final checklist / Common mistakes / Do not change

- SCLK/PICO/CS0 与 DC/BLK 无 Pin 冲突，SPI SCLK 满足屏幕规格。
- 没把 CPU clock 当 SCLK，没把显示刷新率当采样率，没在 ADC/DMA ISR 中画屏。
- 不直接编辑 `.syscfg`/生成文件，不为 waveform helper 新增 Timer/DMA/Event。

## 0. 什么时候用

当上游已经选择好显示窗口的 `float samples[N]` 时使用。24_C 周期信号按硬件频率截取 3 周期，复合信号按 FFT 基波设 X 轴，猝发使用 marker 锁存的完整窗口并优先 min-max envelope；轴、刻度、V/ms 单位和触发判定由应用层提供。

## 1. 为什么需要它

`TFT_ILI9341_DrawLine()` 已经能画线，但比赛里最容易写错的是“如何把 N 点、伏特和时间窗口变成屏幕坐标”。特别是 `N > 屏幕宽度` 时，每隔若干点取一个样本会漏掉窄脉冲和毛刺。

本辅助只封装三件反复出现的事：

1. fixed/auto Y scale；
2. grid、border、baseline；
3. 普通 decimation 或每像素列 min-max envelope。

它不管理 ADC、触发、时基菜单、帧率或双缓冲。

## 2. 两种显示方式怎么选

| 模式 | DEFAULT / USE WHEN | DON'T USE WHEN |
|---|---|---|
| `DECIMATE` | 平滑正弦、N 不远大于屏宽、只想看连贯折线 | 必须保留窄脉冲/毛刺峰值 |
| `MIN_MAX_ENVELOPE` | **数字示波器默认**；方波、脉冲、过冲、N 大于屏宽 | 只追求最细腻的稀疏平滑曲线 |

Envelope 把每个显示列对应的全部样本找出 min/max，再画一条竖线；因此该时间桶里只出现一次的尖峰仍会显示。

## 3. 输入、输出、单位

| 项目 | 内容 |
|---|---|
| 输入 | 已转换、已选择显示窗口的 `float samples[N]` |
| 输入单位 | 可以是 V，也可以是其他线性单位；scale/baseline 必须同单位 |
| 输出 | 直接调用 TFT 基础图元；`result` 返回数据范围、实际 scale 和列数 |
| RAM | O(1)，动态分配 0，无 framebuffer |
| CPU | O(N + 屏宽)；真正耗时通常是阻塞 SPI 绘制 |

## 4. 最小系统链

```text
ADC DMA → ADC To Voltage → Trigger/Timebase 选择 view[start..start+count)
        → SignalTFTWaveform_Draw → ILI9341
```

调用者必须先决定显示哪一段。触发对齐时把 `&voltage[trigger_start]` 和 `view_count` 传入；本模块不会偷偷移动数据。

## 5. 复制与 include

链接唯一源码：

```text
01_bsp/common/signal_status.h
01_bsp/tft_ili9341/（按其 README 的完整驱动、平台和字库清单）
01_bsp/tft_waveform/signal_tft_waveform.h
01_bsp/tft_waveform/signal_tft_waveform.c
```

include：

```c
#include "signal_tft_waveform.h"
```

SysConfig 完全沿用 TFT README 的 SPI/CS/DC/RESET/BL 配置，本辅助不新增 Pin、DMA 或 Timer。

## 6. 初始化和调用

没有 Init；先按 TFT README 完成 `SignalTFTILI9341_MSPM0_Init()`。随后配置绘图区并调用：

```c
signal_tft_waveform_result_t plotted;
signal_tft_waveform_config_t view = {
    .x = 8, .y = 32, .width = 304, .height = 160,
    .mode = SIGNAL_TFT_WAVEFORM_MIN_MAX_ENVELOPE,
    .scale_mode = SIGNAL_TFT_WAVEFORM_FIXED_SCALE,
    .minimum_value = 0.0F,
    .maximum_value = 3.3F,
    .baseline_value = 1.65F,
    .waveform_color = TFT_ILI9341_YELLOW,
    .background_color = TFT_ILI9341_BLACK,
    .grid_color = TFT_ILI9341_BLUE,
    .baseline_color = TFT_ILI9341_CYAN,
    .horizontal_grid_divisions = 4,
    .vertical_grid_divisions = 8,
    .clear_background = true,
    .draw_grid = true,
    .draw_border = true,
    .draw_baseline = true
};

status = SignalTFTWaveform_Draw(&g_tft, &voltage_v[start], view_count,
    &view, &plotted);
```

完整位置初始化版见 [`README_MINIMAL_EXAMPLE.c`](README_MINIMAL_EXAMPLE.c)。

## 7. Fixed scale 与 Auto scale

- `FIXED_SCALE`：示波器 V/div 的默认语义。上下界固定，帧与帧能直接比较；超范围样本只在屏边裁剪，原数组不改。
- `AUTO_SCALE`：快速看清未知幅值。每帧取实际 min/max；平坦信号会自动扩出很小范围，避免除零。

自动量程会让同一幅值在不同帧看起来一样高，不能用像素高度比较绝对幅值。显示测量仪/示波器时优先 fixed；自动只做“找信号”或明确标注 AUTO。

## 8. 显示不能阻塞采集

单帧推荐：

```text
Acquire 完整帧 → Process → Display snapshot → 再启动下一帧
```

连续推荐：

```text
Ping-Pong DMA
  ├─ ISR：只切换 buffer / ready flag
  └─ main：取得 ready frame → Process → 必要时保存 display snapshot
                                → 按较低 UI 频率刷新 TFT
```

不要在 ADC/DMA ISR 里调用任何 TFT 函数。显示刷新率与采样率不是一个参数；TFT 阻塞 SPI 的真实耗时必须上板测量后再定。

## 9. 比赛时最常改什么

| 我要改变 | 参数/位置 | 影响 |
|---|---|---|
| 时间窗 | 调用者的 `start`、`view_count` | X 轴实际时间 = `view_count/Fs` |
| V/div | fixed `minimum_value/maximum_value` | 每格电压由范围与横格数换算 |
| 自动量程 | `scale_mode` | 只改善可见性，不改变测量结果 |
| 方波/毛刺保真 | `MIN_MAX_ENVELOPE` | 不漏每列内 extrema |
| 平滑折线 | `DECIMATE` | SPI 画线观感更连续 |
| 网格 | divisions 与 colors | 只影响显示，不影响算法 |
| 触发位置 | 调用前选择 `start` | 本模块不检测 trigger |

## 10. 常见错误与边界

- 把整个 DMA buffer 传给 TFT 后又立即让 DMA 改写它：先做 snapshot 或遵守 ping-pong 的 Acquire/Release。
- 用简单抽点显示脉冲：换 envelope。
- 用 auto scale 后宣称屏幕高度就是固定 V/div：错误。
- samples 含 NaN/Inf：返回 `SIGNAL_RESULT_NUMERIC_ERROR`，先检查前级。
- 宽高小于 2、fixed max≤min：返回参数错误。
- `N < width`：只绘 N 列并拉伸到整个绘图区，不凭空插入额外样本。

## 11. 验证状态

- PC：`10_tests/pc/build_pipeline_upgrade_debug` 非 `NDEBUG` 构建，95 个库源码编译并完整链接；Y 映射裁剪、equal-bucket 覆盖、单点窄脉冲 envelope、flat auto scale、mock TFT 绘制测试 `1/1 PASS`。
- TI Arm Clang：使用现有 ILI9341 的真实 `SPI_TFT/GPIO_TFT_CTRL` SysConfig profile，将旋钮、TFT 核心、TFT waveform、平台适配和示例 `main` 共 5 个源码编译并最终链接；产物在 `10_tests/ticlang/build_pipeline_upgrade/pipeline_upgrade_tft.out`，结果 `PASS`。
- Board：`NOT_RUN`；屏幕方向、颜色、实际刷新耗时都需实板确认。

## 12. 24_C 成功案例：按信号类型选择 X 轴和绘图模式

本 helper 只负责把稳定的 `float samples[]` 映射到像素；窗口选择和触发策略由应用层完成。24_C 的最终规则如下：

| 信号类型 | 时间轴/窗口 | 绘图策略 |
|---|---|---|
| 周期信号 | 使用 Timer Capture 的硬件频率，截取并显示 3 个周期 | 普通折线或 `MIN_MAX_ENVELOPE` |
| 复合信号 | 使用 FFT 插值后的基波频率设定 X 轴 | 保持采样窗口，显示三周期等效时间轴，并同步显示 H1~H5 |
| 单次猝发 | 使用 marker 起止边沿锁存完整事件窗口（约 1~5 ms） | 优先 `MIN_MAX_ENVELOPE`，不让后续 DMA 覆盖快照 |

绘图区必须自己绘制 X/Y 轴、刻度和单位（周期模式为 `V` 与 `ms`）；`signal_tft_waveform` 不会从频率自动生成轴标签。电压映射前先完成 ADC code -> voltage、去直流和外部调理比例恢复，`minimum_value/maximum_value/baseline_value` 必须与输入数组同单位。

局部刷新时只清除波形矩形并重画波形，数字字段使用固定宽度清除后更新；静态轴线和单位不要每帧重画。输入 buffer 在阻塞绘图期间必须保持稳定，猝发数据应使用独立锁存数组或 ping-pong/triple-buffer 的 display snapshot。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“tft_waveform”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalTFTWaveform_GetEnvelopeColumn -> SignalTFTWaveform_GetModuleStatus -> SignalTFTWaveform_MapY -> SignalTFTWaveform_Draw
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块需要 SysConfig。先在 CCS 的 .syscfg 添加并核对：SPI；再按前文的模块专用 GUI 步骤选择实际 pin/instance。保存后让 CCS 重新生成配置，核对生成宏；不要直接修改 	i_msp_dl_config.c/.h，也不要照抄示例 pin 或 DMA/Event 编号。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_result_t SignalTFTWaveform_MapY(float value, float scale_minimum, float scale_maximum, int32_t plot_y, uint16_t plot_height, int32_t *screen_y);`

**它做什么：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `value` | `float` | `value`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `scale_minimum` | `float` | `scale_minimum`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `scale_maximum` | `float` | `scale_maximum`（`float`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `plot_y` | `int32_t` | `plot_y`（`int32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `plot_height` | `uint16_t` | `plot_height`（`uint16_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `screen_y` | `int32_t *` | `screen_y`（`int32_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 当前实现中出现的返回/成熟度枚举值：`SIGNAL_RESULT_INVALID_ARGUMENT`、`SIGNAL_RESULT_OK`。

**最小调用形状：** `SignalTFTWaveform_MapY(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalTFTWaveform_GetEnvelopeColumn(const float *samples, size_t sample_count, uint16_t column_count, uint16_t column, float *minimum, float *maximum);`

**它做什么：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `samples` | `const float *` | 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。 |
| `sample_count` | `size_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |
| `column_count` | `uint16_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |
| `column` | `uint16_t` | `column`（`uint16_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `minimum` | `float *` | `minimum`（`float *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `maximum` | `float *` | `maximum`（`float *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 当前实现中出现的返回/成熟度枚举值：`SIGNAL_RESULT_INVALID_ARGUMENT`。

**最小调用形状：** `SignalTFTWaveform_GetEnvelopeColumn(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalTFTWaveform_Draw(tft_ili9341_t *tft, const float *samples, size_t sample_count, const signal_tft_waveform_config_t *config, signal_tft_waveform_result_t *result);`

**它做什么：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `tft` | `tft_ili9341_t *` | `tft`（`tft_ili9341_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `samples` | `const float *` | 调用者持有的数据或缓冲区。容量、单位和读写方向以函数名及本 README 的输入输出说明为准；异步硬件操作完成前不得改写。 |
| `sample_count` | `size_t` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |
| `config` | `const signal_tft_waveform_config_t *` | 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。 |
| `result` | `signal_tft_waveform_result_t *` | 由调用者分配的输出对象/数组。成功返回后才读取其中内容。 |

**返回：** 当前实现中出现的返回/成熟度枚举值：`SIGNAL_RESULT_INVALID_ARGUMENT`、`SIGNAL_RESULT_OK`。

**最小调用形状：** `SignalTFTWaveform_Draw(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalTFTWaveform_GetModuleStatus();`

**它做什么：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 返回 signal_module_status_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalTFTWaveform_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

## 18. 比赛通用功能代码

本 README 已提供 fixed/auto scale、`MapY()`、envelope 和局部波形绘制；显示范围计算、
动态 X 轴时间值和“量程变化时才重画轴标签”属于应用层。请复制统一手册
[CONTEST_FUNCTIONAL_CODE_COOKBOOK.md](../../00_docs/CONTEST_FUNCTIONAL_CODE_COOKBOOK.md)
的第 3 节。不要用自动量程后的像素高度反推真实 Vpp，测量仍使用原始电压数组。

## 19. 可直接复制：动态坐标轴与局部刷新

`SignalTFTWaveform_Draw()`、`MapY()` 和 envelope 是本模块提供的绘图算法；下面两个
函数是 `main.c` 自写的显示范围组合逻辑。它们只决定“如何把当前帧映射到屏幕”，不会
改动 ADC 原始数据或测量结果。

```c
static bool App_SelectDisplayScale(const float *samples,
                                   size_t count,
                                   float *minimum_value,
                                   float *maximum_value)
{
    size_t i;                                     /* 当前样本下标。 */
    float minimum;                                /* 原始最小值。 */
    float maximum;                                /* 原始最大值。 */
    float span;                                   /* 原始峰峰范围。 */
    float margin;                                 /* 上下显示余量。 */

    if ((samples == NULL) || (count == 0U) ||
        (minimum_value == NULL) || (maximum_value == NULL)) {
        return false;                             /* 参数不完整时不写输出。 */
    }
    minimum = samples[0];                         /* 用首点初始化范围。 */
    maximum = samples[0];
    for (i = 1U; i < count; ++i) {                /* 扫描剩余样本。 */
        if (samples[i] < minimum) minimum = samples[i];
        if (samples[i] > maximum) maximum = samples[i];
    }
    span = maximum - minimum;                     /* 计算峰峰值。 */
    if (span < 0.1F) {                            /* 平坦信号避免除零。 */
        minimum -= 0.05F;
        maximum += 0.05F;
    } else {
        margin = span * 0.1F;                     /* 上下各留 10% 空白。 */
        minimum -= margin;
        maximum += margin;
    }
    *minimum_value = minimum;                     /* 输出 Y 轴下限。 */
    *maximum_value = maximum;                     /* 输出 Y 轴上限。 */
    return true;                                  /* 量程有效。 */
}

static float App_GetWindowMilliseconds(size_t display_count,
                                       uint32_t actual_sample_rate_hz)
{
    if ((display_count == 0U) || (actual_sample_rate_hz == 0U)) {
        return 0.0F;                              /* 分母或点数无效。 */
    }
    return 1000.0F * (float)display_count /
        (float)actual_sample_rate_hz;             /* X 轴窗口：N/Fs*1000。 */
}
```

调用 `SignalTFTWaveform_Draw()` 前先计算量程；只有量程、窗口或页面变化时才重画轴线、
刻度和单位。每帧只清理固定波形矩形（`FillRect`）再重画波形，不能在 ADC/DMA ISR 中
调用任何 TFT API。`minimum_value` 和 `maximum_value` 只用于显示，Vpp、频率和相位仍
从未缩放的电压数组计算。

