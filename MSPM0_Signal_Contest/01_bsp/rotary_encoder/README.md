# Rotary Encoder：旋转编码器解码模块

状态：`BUILD_VERIFIED`；Board：`NOT_RUN`。本模块复用外部器件教程 [EC11 类机械旋转编码器](../../12_external_devices/input_devices/ec11/README.md)，不假定任何具体 EC11 板的引脚顺序。

## CCS SysConfig GUI Configuration

### Required resources

只需要 `GPIO`：A、B 两个 input，可选 SW input。时钟状态：`NO_INDEPENDENT_PERIPHERAL_CLOCK`；Gray 解码和按键 debounce 的调用周期属于应用层。

### Step 1 - Add A/B and optional SW

GUI Path: 左侧 `Software` -> `Add` -> `GPIO` -> 实例 -> `Group Pins` -> `ADD`；为 A、B 和可选 SW 各建一行，设置 `Name`、`Direction = Input`、`IO Structure`，再按 EC11 接线分配 Port/Pin。

### Step 2 - Input resistor and polarity

GUI Path: 左侧 `GPIO` -> A/B/SW 对应 pin -> `Digital IOMUX Features` -> `Internal Resistor`，按 EC11 原理图选择 `Pull-up` 或 `Pull-down`；同页 `Invert` 与软件 active-low 约定只能启用一处。

### Step 3 - PinMux and clock

GUI Path: 每个 pin -> `Assigned Port` / `Assigned Port Segment` / `Assigned Pin` -> `PinMux Peripheral and Pin Configuration`。Action: 核对 A/B/SW 的物理 pin 无冲突。Clock: `NO_INDEPENDENT_PERIPHERAL_CLOCK`；不为普通轮询解码新增 Timer。概念边界见[共享时钟教材](../../00_docs/MSPM0_SYSCONFIG_CLOCK_TIMER_ADC_DAC_BEGINNER_GUIDE.md)。

### Expected generated symbols

核对 GPIO group `*_PORT`、A/B/SW `*_PIN` 和 `*_IOMUX` 宏。PROJECT_AUDIT 记录 `Name/Direction/Internal Resistor/Assigned Pin -> .syscfg -> generated symbol`。

### Final checklist / Common mistakes / Do not change

- A/B 顺序与旋转方向期望一致，非法跳变没有被误当有效步进。
- 输入无浮空，软件和 GUI 未双重反相；没有无故创建 Timer。
- 不直接编辑 `.syscfg`/生成文件，不假定 EC11 排针。

## 1. 它解决什么问题

机械旋钮通常有 A、B、公共端 C/COM，带按压的型号还有 SW。转动时 A/B 按 Gray 状态顺序变化：

```text
GPIO A/B → 状态表解码 → transition accumulator → -1 / +1 step
GPIO SW  → 连续多次一致 → pressed / released
```

它只做小而清晰的输入解码，不包含菜单、参数上下限、加速曲线或屏幕刷新。

## 2. 输入与输出

| 项目 | 内容 |
|---|---|
| 输入 | A、B 电气高低电平；可选 SW 电平 |
| 输出 | `step_delta`（-1/0/+1）、累计 `position`、按下/松开事件、非法跳变计数 |
| 单位 | step；没有电压单位 |
| 动态内存 | 0 |

## 3. 接线与第一次确认

| 编码器端 | MSPM0G3507 | 小白说明 |
|---|---|---|
| A | GPIO input | 方向判断的第一路触点 |
| B | GPIO input | 方向判断的第二路触点 |
| C/COM | 通常 GND | 必须先用万用表确认实物公共端 |
| SW | GPIO input | 按下旋钮的独立按键，可没有 |

裸编码器常用内部上拉；模块板可能已经带上拉和 RC。先查实物，不能同时凭空加相反方向的上下拉。若方向与预期相反，最简单的做法是交换 A/B，而不是改正式源码。

## 4. SysConfig 怎么配

1. 建一个 GPIO 组，例如 `ENCODER`。
2. A、B、SW 配成 Digital Input。
3. 公共端接 GND 时通常启用 pull-up；模块自带上拉时按板卡原理图决定。
4. 第一次只用轮询，不需要 IRQ。
5. Build 后必须从 `ti_msp_dl_config.h` 抄实际生成宏；README 示例中的 `ENCODER_A_PORT` 等只是你应在 SysConfig 中得到的命名形式。

只有快速转动确实丢步时才升级 GPIO interrupt。ISR 只保存电平/事件，菜单和 TFT 刷新仍放主循环。

## 5. 复制和加入工程

复制或链接唯一源码：

```text
01_bsp/common/signal_status.h
01_bsp/rotary_encoder/signal_rotary_encoder.h
01_bsp/rotary_encoder/signal_rotary_encoder.c
```

把本目录和 `01_bsp/common` 加到 include path。不要复制第二份后再修改。

## 6. 初始化和调用

公开 API：

```c
SignalRotaryEncoder_Init(...);
SignalRotaryEncoder_Update(...);
SignalRotaryEncoder_GetPosition(...);
SignalRotaryEncoder_SetPosition(...);
SignalRotaryEncoder_GetInvalidTransitionCount(...);
```

完整可复制骨架见 [`README_MINIMAL_EXAMPLE.c`](README_MINIMAL_EXAMPLE.c)。三个回调只负责读取真实 GPIO 高低电平。主循环按固定节拍调用 `Update`：

```c
if (SignalRotaryEncoder_Update(&g_encoder, &event) == SIGNAL_RESULT_OK) {
    selected_value += event.step_delta;
    if (event.button_pressed) {
        /* 确认 selected_value。 */
    }
}
```

## 7. 比赛时最常改什么

| 我要改变 | 改哪里 | 说明 |
|---|---|---|
| A/B/SW 引脚 | `.syscfg` | 重新 Build 后用生成宏 |
| 每格多少次状态变化 | `transitions_per_step` | 常见 1、2、4；用串口观察一格再定 |
| 按键去抖 | `button_debounce_scans` 与轮询周期 | 例如 3 次×5 ms 只是示例，不是固定值 |
| 方向 | 交换 A/B | 不改库源码 |
| 数值上下限/步长 | Application | 本模块只输出 step |

## 8. 常见错误

- 把 `transitions_per_step` 当成“每圈格数”。它表示一个逻辑 step 需要几个有效 A/B 状态变化。
- 轮询太慢会从 00 直接跳到 11；模块会记为 `invalid_transition`，无法恢复被漏掉的方向。
- 触点抖动严重时先降低轮询间隔、确认硬件 RC，再考虑 IRQ；不要在 ISR 里画屏。
- 初始化时旋钮正处于触点抖动区，第一次状态可能不稳；上电后转一格做交互确认即可。

## 9. 验证

- PC：`10_tests/pc/build_pipeline_upgrade_debug` 非 `NDEBUG` 构建，95 个库源码编译并完整链接；正向/反向 Gray 序列、00→11 非法跳变、累计位置、按键消抖、参数边界测试 `1/1 PASS`。
- TI Arm Clang：使用现有 ILI9341 的真实 `SPI_TFT/GPIO_TFT_CTRL` SysConfig profile，将旋钮、TFT 核心、TFT waveform、平台适配和示例 `main` 共 5 个源码编译并最终链接；产物在 `10_tests/ticlang/build_pipeline_upgrade/pipeline_upgrade_tft.out`，结果 `PASS`。
- Board：`NOT_RUN`，接真实编码器后才能升级。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“rotary_encoder”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalRotaryEncoder_Init -> SignalRotaryEncoder_SetPosition -> SignalRotaryEncoder_Update -> SignalRotaryEncoder_GetPosition -> SignalRotaryEncoder_GetInvalidTransitionCount -> SignalRotaryEncoder_GetModuleStatus
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块需要 SysConfig。先在 CCS 的 .syscfg 添加并核对：GPIO；再按前文的模块专用 GUI 步骤选择实际 pin/instance。保存后让 CCS 重新生成配置，核对生成宏；不要直接修改 	i_msp_dl_config.c/.h，也不要照抄示例 pin 或 DMA/Event 编号。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_result_t SignalRotaryEncoder_Init(signal_rotary_encoder_t *encoder, const signal_rotary_encoder_config_t *config);`

**它做什么：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

**什么时候调用：** 根据调用者提供的配置或对象完成一次初始化；通常在 `SYSCFG_DL_init()` 之后且仅调用一次。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `encoder` | `signal_rotary_encoder_t *` | `encoder`（`signal_rotary_encoder_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `config` | `const signal_rotary_encoder_config_t *` | 调用者填写的配置对象。先阅读该类型的成员；它控制本次初始化或处理方式。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalRotaryEncoder_Init(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalRotaryEncoder_Update(signal_rotary_encoder_t *encoder, signal_rotary_encoder_event_t *event);`

**它做什么：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

**什么时候调用：** 执行该模块公开的功能；具体数据流以本节参数表和本 README 前面的场景说明为准。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `encoder` | `signal_rotary_encoder_t *` | `encoder`（`signal_rotary_encoder_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `event` | `signal_rotary_encoder_event_t *` | `event`（`signal_rotary_encoder_event_t *`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 当前实现中出现的返回/成熟度枚举值：`SIGNAL_RESULT_OK`、`SIGNAL_RESULT_INVALID_ARGUMENT`、`SIGNAL_RESULT_NOT_INITIALIZED`。

**最小调用形状：** `SignalRotaryEncoder_Update(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalRotaryEncoder_GetPosition(const signal_rotary_encoder_t *encoder, int32_t *position);`

**它做什么：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `encoder` | `const signal_rotary_encoder_t *` | `encoder`（`const signal_rotary_encoder_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `position` | `int32_t *` | `position`（`int32_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalRotaryEncoder_GetPosition(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalRotaryEncoder_SetPosition(signal_rotary_encoder_t *encoder, int32_t position);`

**它做什么：** 修改模块的一个运行参数；若模块有 BUSY/RUNNING 状态，应在空闲时修改。

**什么时候调用：** 修改模块的一个运行参数；若模块有 BUSY/RUNNING 状态，应在空闲时修改。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `encoder` | `signal_rotary_encoder_t *` | `encoder`（`signal_rotary_encoder_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `position` | `int32_t` | `position`（`int32_t`）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalRotaryEncoder_SetPosition(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_result_t SignalRotaryEncoder_GetInvalidTransitionCount(const signal_rotary_encoder_t *encoder, uint32_t *count);`

**它做什么：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

**什么时候调用：** 读取最近一次操作保存的状态、结果或配置；先确认前置操作已经成功。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `encoder` | `const signal_rotary_encoder_t *` | `encoder`（`const signal_rotary_encoder_t `）是该 API 的输入/输出参数；按本 README 前面的数据单位和边界条件准备。 |
| `count` | `uint32_t *` | 元素数量或容量，单位是“元素个数”而不是字节；必须与实际数组大小一致。 |

**返回：** 返回 signal_result_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalRotaryEncoder_GetInvalidTransitionCount(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### `signal_module_status_t SignalRotaryEncoder_GetModuleStatus();`

**它做什么：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**什么时候调用：** 读取模块当前的验证成熟度或静态状态，不会启动硬件操作。

**参数：** 无。

**返回：** 返回 signal_module_status_t 类型结果；调用者应检查该值。

**最小调用形状：** `SignalRotaryEncoder_GetModuleStatus(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

