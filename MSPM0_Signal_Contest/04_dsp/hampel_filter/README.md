# HampelFilter：用局部 MAD 判断毛刺

> **LEVEL C / REAL ALGORITHM MODULE：** 每点局部中位数/MAD、workspace、窗口边界和“不能误删真实尖峰”的风险使它不适合现场临时重写。

**比赛复制清单：** `signal_hampel.c`、`signal_hampel.h`、`04_dsp/mad/signal_mad.c`、`signal_mad.h`、`03_measurement/common/signal_algorithm_status.h`。需要调用者提供至少 `window_size` 个 float workspace；无 SysConfig/Pin。

## 1 这个算法是干什么的？

它逐点观察附近窗口：先找局部中位数，再用 MAD 判断当前点离“邻居多数”是否太远。太远就把它替换成局部中位数。

## 2 一个最简单的例子

`1,1,1,100,1,1,1`，窗口 5。100 周围的中位数是 1，MAD 为 0，100 明显离群，被替换成 1；其余点保留。

## 3 原理

局部尺度 `sigma≈1.4826*MAD`；若 `|x-median| > threshold_sigma * max(sigma, minimum_scale)`，判为离群。Hampel 能去毛刺，是因为中位数和 MAD 都不容易被少量极端值拖走。

## 4 比赛里什么时候用？

已经确认存在偶发 ADC 错码/通信毛刺，并且目标是稳定 Vpp、RMS、频率，而不是测尖峰本身。

## 5 输入

输入/输出 float 数组、count、奇数 window、正 `threshold_sigma`、非负 `minimum_scale`（与输入同单位）、workspace 至少 W 个 float。

## 6 输出

滤波数组和被替换点数。单位不变，输入输出不得重叠。

## 7 API怎么调用

```c
signal_hampel_config_t cfg = {7U, 3.0f, 0.001f};
float work[7];
signal_hampel_result_t r;
SignalHampel_Process(in_v, out_v, count, &cfg, work, 7U, &r);
```

## 8 参数怎么改

窗口先 5/7；`threshold_sigma` 常从 3 开始；`minimum_scale` 设为可接受的最小噪声尺度（V），避免量化平坦区阈值完全为 0。

## 9 参数改大会怎样

- window 大：尺度更稳定、能看更宽背景，但真实变化更容易被当异常，CPU 增加。
- threshold 大：替换更少、漏毛刺增加；小：更激进、误删增加。
- minimum_scale 大：更不容易在平坦区替换小偏差，但也可能放过小毛刺。

## 10 这个算法的代价是什么

Benefits：局部自适应、少量极端值鲁棒、输出替换计数可诊断。

Trade-offs：非线性、计算比 Median 更复杂、需要输出/workspace，可能制造“过分干净”的假波形。

## 11 什么时候不要用

尖峰、故障脉冲、开关瞬态、burst、方波边沿是题目目标时绝对不要用。THD/频谱前也不能默认加入。

## 12 怎么和前一个模块接

```text
ADC_ToVoltage -> Hampel
```

## 13 怎么和后一个模块接

```text
┌──── Hampel ────┐
│ local median   │
│ local MAD      │
│ replace outlier│
└───────┬────────┘
        ├──> RobustVPP / RMS
        └──> ZeroCross
```

## 14 最小Demo

```c
float x[]={1,1,1,100,1,1,1}, y[7], w[5];
signal_hampel_config_t c={5U,3.0f,0.0f};
signal_hampel_result_t r;
(void)SignalHampel_Process(x,y,7U,&c,w,5U,&r);
```

## 15 PC测试

上例 Expected 七点全为 1，`replaced_count=1`；Measured 全一致，PASS。

排查：替换太多先画原始数据并增加 threshold/minimum_scale；毛刺漏掉检查窗口内异常比例是否过高；边沿变形说明算法不适合该信号。

## 16 MCU资源

输出 `4N`、workspace `4W`；平均约 O(N·W)，含大量比较和绝对值。建议 W 小且按帧离线运行，不在 ISR 内调用。

## 17 验证状态

PC_VERIFIED：2026-08-07，严格编译、毛刺和计数真值测试通过；未 BOARD_VERIFIED。

## 17. 统一 API 教程（已按当前头文件核对）

本节由当前公开头文件、实现中实际出现的状态码和正式模块注册表生成。它补充前文的场景教程；函数签名变化时必须重新运行 `tools/upgrade_formal_beginner_docs.ps1`，不要手工保留旧 API。

遵循仓库的 [Beginner README 标准](../../00_docs/BEGINNER_README_STANDARD.md)：先用最小示例完成一次正常数据流，再按需要阅读全功能示例和本节 API 细节。

### 什么时候用 / 什么时候不要用

当题目需要“hampel_filter”目录对应的公开功能，并且输入数据、单位和硬件资源满足前文约束时使用本模块。若只需要更简单的上游功能、输入尚未准备好，或需要不同的数据模型/外设资源，应先选择相邻模块而不是强行调用本 API。

### 输入 / 输出

输入由各 API 的只读数组、配置、频率/阈值和平台对象组成；输出写入 result/output/buffer 参数或由 Get API 返回。调用者负责数组容量、生命周期和物理单位；失败返回时输出不是有效结果。

### 调用顺序

```text
SignalHampel_Process
```

`Init` 一般仅一次；`Set/Configure` 仅在参数变化且模块空闲时执行；`Start/Process/Generate` 是每帧或每次任务入口；`Get/Is` 用于读取已完成的结果；`Stop` 只在需要取消时调用。若本模块没有其中某类 API，以实际列出的函数为准。

### SysConfig 边界

本模块是纯软件/算法模块，**不需要 SysConfig**。ADC、DAC、Timer、DMA、引脚和时钟由上游模块配置；调用时只把真实的采样率、数组长度、单位等事实传入。

### 参数分级

- 【比赛必须会】输入/输出数组、`count/length/capacity`、采样率/频率、阈值/增益以及本 README 前文标出的 pin。它们直接影响题目范围、RAM、时间轴或物理单位。
- 【出问题再理解】Timer 时钟、DMA 通道、Event 路由、参考源和 IRQ。它们属于硬件链路，必须与 SysConfig 生成结果一致。
- 【以后进阶】多缓冲、运行时重配置、回调调度和 ISR 优化。先用最小示例完成一帧闭环，再处理吞吐或延迟。

### 常见错误 FAQ

- 参数错误：先检查指针非空、count/capacity 的单位是元素数、频率/阈值单位与上游一致。
- 硬件无结果：不要修改生成文件；回到 SysConfig 核对 pin、instance、时钟、Timer、DMA 和 Event 的完整链路。
- 结果异常：确认上一轮异步采集已经完成，真实 Fs/N/参考电压已传到算法，且没有在 DMA 使用期间改写 buffer。

### `signal_algorithm_status_t SignalHampel_Process(const float *input_samples, float *output_samples, uint32_t count, const signal_hampel_config_t *config, float *workspace, uint32_t workspace_count, signal_hampel_result_t *result);`

**它做什么：** 用局部中位数和 MAD 检测孤立离群点，并以局部中位数替换。

**什么时候调用：** 对调用者提供的数据执行一次同步计算或生成，并在成功后写入输出对象/数组。

| 参数 | 类型 | 初学者解释 |
|---|---|---|
| `input_samples` | `const float *` | 输入数组，只读；不得与 output_samples 重叠。 |
| `output_samples` | `float *` | 输出数组，容量至少 count，单位与输入相同。 |
| `count` | `uint32_t` | 样本点数。 |
| `config` | `const signal_hampel_config_t *` | 奇数窗口、sigma 阈值、与输入同单位的最小尺度。 |
| `workspace` | `float *` | 临时 float 数组，会被重排覆盖。 |
| `workspace_count` | `uint32_t` | 临时容量，至少为 window_size。 |
| `result` | `signal_hampel_result_t *` | 输出被替换点数。 |

**返回：** 成功返回 SIGNAL_ALGORITHM_OK；参数、空间或数值非法返回错误码。

**最小调用形状：** `SignalHampel_Process(...);`。可直接从 README_MINIMAL_EXAMPLE.c 复制正常流程；README_FULL_EXAMPLE.c 展示全部公开 API，其中取消类 API 会以 #if 0 隔离。

**注意：** 所有指针和数组都由调用者拥有；先检查返回值。异步采集、DMA 或回调还在使用 buffer 时，不能读取结果或改写该 buffer。

### 示例、模块链与验收

- 最小入门：`README_MINIMAL_EXAMPLE.c`，只保留正常入口和结果读取。
- 全功能：`README_FULL_EXAMPLE.c`，以正确顺序展示当前头文件全部公开 API；`Stop` 等非常规路径不会默认执行。
- 模块链：先由上游提供单位、采样率和有效数据，再调用本模块；成功后将输出交给显示、控制、测量或下一步 DSP。硬件资源仍以 SysConfig 合约为唯一来源。
- 文档验收：README/API、两份示例和头文件会由 `tools/validate_beginner_documentation.ps1` 覆盖检查；这只表示文档与源码签名一致，不代替未进行的实板验证。

### 模块链

`上游采集/配置 -> 本模块 -> 检查返回值和结果 -> 测量、显示、控制或下一步 DSP`。只有确认本模块的输出单位和有效状态后，才交给下一模块。

