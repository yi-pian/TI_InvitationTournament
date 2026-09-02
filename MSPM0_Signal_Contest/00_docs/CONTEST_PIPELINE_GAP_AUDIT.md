# Contest Pipeline Gap Audit

日期：2026-08-13。本文只记录“Pipeline 能否交付”的真实缺口；目录、README 或算法名存在不等于完整仪器已完成。

## 1. 判断口径

| 状态 | 含义 |
|---|---|
| `COVERED` | 已有真实 Module/Recipe/Application 可以组成该节点，仍需按各自状态验证 |
| `RECIPE_ONLY` | 逻辑链和可复制代码已写清，但没有完整 CCS Application |
| `BUILD_REFERENCE` | 已有完整 Application 的 SysConfig/Compile/Link 证据，可作组合参考；Board 仍可能 `NOT_RUN` |
| `GAP` | 当前缺少独立可验证实现、精确外设驱动或完整系统闭环 |

所有新显示/HMI内容的 Board 状态均为 `NOT_RUN`。

## 2. 本轮已经补上的 P0 缺口

| 能力 | 本轮结果 | 级别决定 | 真实状态 |
|---|---|---|---|
| N 点波形画到 ILI9341 | `01_bsp/tft_waveform`：抽点、min-max envelope、fixed/auto scale、grid/baseline | 升级为小型 Module；它有清晰 API、重复使用且可独立测试 | `PC_VERIFIED + BUILD_VERIFIED`，Board `NOT_RUN` |
| 旋钮调连续参数 | `01_bsp/rotary_encoder`：A/B Gray 解码、step、位置、SW 去抖 | 升级为小型 Module；状态与去抖值得复用 | `PC_VERIFIED + BUILD_VERIFIED`，Board `NOT_RUN` |
| 波形显示逻辑 | `WAVEFORM_DISPLAY_RECIPE.md` | Recipe；不再造示波器框架 | `RECIPE_ONLY` |
| 频谱柱/峰标记 | `SPECTRUM_DISPLAY_RECIPE.md` | Recipe；只做坐标映射和显示语义 | `RECIPE_ONLY` |
| XY/Bode 曲线 | `XY_BODE_DISPLAY_RECIPE.md` | Recipe；短映射逻辑不值得独立 `.c/.h` | `RECIPE_ONLY` |
| 示波器/运放/分类/HMI选择 | 四份专项 Guide | Guide；负责系统顺序、选择边界和失效条件 | 文档检查完成 |

## 3. 仍需补齐的 P0

| GAP | 当前已有 | 缺什么 | 决定 | 为什么不在本轮硬造 |
|---|---|---|---|---|
| 可上板数字示波器 Application | Trigger/Ring/Ping-Pong、测量 Recipe、TFT/TFT Waveform 都在 | 一个真实 `.syscfg + projectspec + main`，把 Acquire/Process/Display/HMI ownership 闭环 | **New Application** | 需要确定 ADC 通道、SPI/Pin、Timer/DMA/Event ownership 和目标刷新率；不能由文档冒充 |
| 带 TFT 的 Spectrum Analyzer | Build-verified spectrum core、频谱显示 Recipe、TFT driver | 采集与 TFT 同 profile 的完整工程和实板刷新验证 | **New Application** | 算法无需新模块；缺的是系统资源与显示调度 |
| 带 TFT/HMI 的 Frequency Response Analyzer | `sweep_analyzer` Build reference、Bode Recipe | 双通道/参考通道策略、校准、屏幕与旋钮完整工程 | **Extend/New Application** | 是否用单参考、双 ADC 或外部仪器会改变硬件事实 |
| 自动运放测试仪 | gain/bandwidth/slew/THD Recipe 与 sweep reference | 安全激励、量程、夹具、DUT 电源/负载、完整状态机 | **New Application + fixture** | 关键缺口是模拟夹具和保护，不是再造算法名 |
| ADC+DAC+TFT+HMI 资源闭环 | 各子模块与若干 Resource Profile | 目标 Pin/Timer/DMA/Event 无冲突的统一 profile 与上板结果 | **Application/Profile validation** | 一个 SysConfig profile 不能证明所有板级连线都可用 |

## 4. P1：特定赛题再升级

| GAP | 现状 | 推荐升级层级 | 触发升级条件 |
|---|---|---|---|
| 波形自动分类 | Guide 给出 feature chain；FFT、RMS、Vpp、harmonic 可复用 | 先 Recipe/Application；分类器只有反复使用且有数据集/golden model 时才做 Module | 题目明确类别、噪声、幅频范围与准确率 |
| 信号失真诊断 | THD/Harmonic/SNR Recipe 已覆盖数值；“削顶/交越/压缩”诊断未验证 | Recipe + Application | 题目明确失真类型与判据 |
| AM/FM/CW 识别或分离 | 基础频谱、包络/相位 Primitive 可候选 | Recipe；复杂盲分离保持 `GAP` | 明确调制方式、载波范围、信噪比、实时要求 |
| 自动增益/自动量程闭环 | automatic_gain Recipe、PGA/VGA/数字电位器器件层 | Exact-device Application | 确定可编程增益器件、settling、标定 LUT 和安全阈值 |
| 程控滤波器 | FIR/IIR 算法与外部开关/数字电位器指南 | Exact-device Application | 确定是数字滤波还是模拟 R/C 切换及具体器件 |
| Capture/Replay 的屏幕编辑 | Build-verified capture/replay core | Extend Application | 题目要求波段选择、缩放、编辑或触发预览 |
| SPWM | SysConfig + Timer/PWM + 短 Recipe 足够 | 暂不建 Module | 只有死区、多相同步、调制表更新反复出现才升级 |

## 5. P2：高级/研究型缺口

- 盲源分离、未知调制的通用分类器、协议无关解调器：当前 `GAP`；需要明确数据集、指标和 PC golden model。
- 高级示波器自动测量集合、数字余辉、深存储分段：当前 `GAP`；MSPM0G3507 RAM 与 TFT 刷新预算必须先实测。
- 自动器件参数全套 ATE：当前 `GAP`；需要继电器/模拟开关矩阵、保护、校准基准和夹具，不应只靠 MCU 软件模块化。

## 6. 验证工具缺口

`10_tests/ticlang/validate_link.ps1` 会把所有平台条件源码塞进同一个 ADC SysConfig profile。本轮真实执行在已有 `signal_tft_ili9341_mspm0g3507.c` 处因该 profile 没有 `SPI_TFT_INST/GPIO_TFT_CTRL_*` 而停止。这说明聚合脚本需要按 Resource Profile 分组，不说明 TFT 或 ADC 应共用一个错误 profile。

决定：保持正式模块不变；后续把 TI 验证拆成 ADC、DAC、TFT、Dual ADC 等 profile group。当前两个新模块已在真实 TFT profile 下完成 5 源码最终链接，但隔离 COPY TEST 和 Board 均未运行。

