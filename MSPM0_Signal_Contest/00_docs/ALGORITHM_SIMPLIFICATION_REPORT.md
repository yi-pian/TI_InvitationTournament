# 算法库简化审查报告

审查日期：2026-08-11。范围是实际存在于 `03_measurement/`、`04_dsp/`、`05_precision/` 的源码和 TODO；没有按旧规划凭空补模块。

## 结论

| 处理级别 | 数量 | 含义 |
|---|---:|---|
| Level A：Direct Recipe | 10 个现有实现目录 | 新工程不再推荐复制 `.c/.h`；详细直接代码进入 Cookbook/Recipe，旧 API 仅兼容 |
| Level B：Simple Helper | 2 | 保留单一无状态 Helper，不增加 Init/context |
| Level C：Real Algorithm Module | 28 | 继续作为正式复制入口 |
| Internal / Child Implementation | 5 | 不作为用户选择入口 |
| DRAFT / TODO | 5 | 没有可调用源码，不伪装成正式模块 |

实现目录合计 45 个；另有 5 个纯设计/TODO 目录。

## Level A：Direct Recipe

| 原目录 | 新推荐 | 为什么降级 | 旧 `.c/.h` |
|---|---|---|---|
| `03_measurement/mean` | `recipes/mean.md` | 一次求和和除法；无状态/workspace | 冻结兼容 |
| `03_measurement/minmax` | `recipes/minmax.md` | 一次比较循环即可得到 min/max | 冻结兼容 |
| `03_measurement/vpp` | `recipes/vpp.md` | `max-min`，模块结果结构增加了现场成本 | 冻结兼容 |
| `03_measurement/rms` | `recipes/rms.md` | 平方、平均、开方；边界简单 | 冻结兼容 |
| `03_measurement/ac_rms` | `recipes/ac_rms.md` | 求均值后再算偏差平方；无状态 | 冻结兼容 |
| `03_measurement/adc_to_voltage` | `recipes/adc_to_voltage.md` | 固定线性换算循环；参数可直接集中在应用 config | 冻结兼容 |
| `04_dsp/remove_dc` | `recipes/remove_dc.md` | 两个短循环，可安全原地处理 | 冻结兼容 |
| `04_dsp/clipping_detect` | `recipes/clipping_detect.md` | 阈值比较和计数；不值得 config/result 层 | 冻结兼容 |
| `04_dsp/peak_detect` | `recipes/peak_detect.md` | 指定范围找最大值是一个短循环 | 冻结兼容 |
| `05_precision/multi_cycle_average` | `recipes/multi_cycle_average.md` | 同向过零首尾位置即可直接求平均周期 | 冻结兼容 |

另外四项目前没有正式实现目录，也只提供 Direct Recipe：Scaling、Offset Correction、Normalize、Threshold。不会为了目录整齐新增四套 `.c/.h`。

## Level B：Simple Helper

| 目录 | 保留原因 | API 约束 |
|---|---|---|
| `04_dsp/moving_average` | 运行和、起始边界和禁止原地的语义值得统一 | 单次 `Process(input, output, count, window_size)`；无 Init/context/result |
| `05_precision/window_gain_correction` | 单边谱中 DC/Nyquist 不乘 2，其他 bin 乘 2；现场重复写容易标度错误 | 单次 `Apply(raw, output, bins, N, coherent_gain)`；允许原地 |

## Level C：Real Algorithm Module

| 类别 | 正式模块 | 保留原因 |
|---|---|---|
| 测量 | Statistics、Zero Cross、Phase | 多输出；滞回/事件容量；多种相位来源和符号环绕 |
| 变换与频谱 | FFT、FFT Magnitude、Window Dispatcher、Multi-Bin Energy | Backend/复数格式/幅值语义/窗增益与频谱边界容易写错 |
| 峰值精度 | FFT Parabolic、Log Parabolic、Zero-Cross Interpolation | 邻点边界、退化分母、fractional bin/采样位置语义重要 |
| 滤波与异常值 | Median、MAD、Hampel、FIR、IIR Biquad | workspace、状态、系数、数值稳定或真实瞬态误删风险 |
| 周期与相关 | Correlation、Autocorrelation | lag 约定、归一化、搜索范围和 O(N·lag) 资源风险 |
| 谐波与质量 | Harmonic、THD、SNR、SFDR | 能量/幅值不能混用，需处理基波、谐波窗口和排除区 |
| 精度/校准 | ADC Gain/Offset Calibration、Channel Delay Calibration、Robust VPP、Robust RMS | 校准参数、多点/分位数/workspace和物理含义需要统一 |
| 拟合/弱信号 | Sine Fit 3、Sine Fit 4、Lock-In | 最小二乘、局部搜索、初值、参考相位和残差判断复杂 |

## Internal / Child Implementation

以下目录不再作为比赛现场选择入口：

- `04_dsp/backend_adapter`：Backend 内部数据格式适配。
- `04_dsp/window/hann`
- `04_dsp/window/hamming`
- `04_dsp/window/blackman`
- `04_dsp/window/rectangular`

正常应用只选择 `04_dsp/window` 的统一 `SignalWindow_Apply()`。四个子目录只保留兼容别名，不应和父模块并列选择。

## DRAFT / TODO

`CZT`、`Zoom FFT`、`Jacobsen`、`Quinn`、`Macleod` 只有设计/TODO，没有 `.c/.h`。它们继续保持 `DRAFT`，不进入比赛现场正式选择表。

## 没有做的事情

- 没有删除旧兼容源码，因此现有 Application 不会因本轮文档简化突然失效。
- 没有修改 FFT/CMSIS-DSP/IQMath Backend。
- 没有把 Recipe 再包装成新的“万能简单算法模块”。
- 没有把 PC/完整链接结果冒充板级验证。

## README 与代码验证结果

| 检查 | 结果 |
|---|---|
| 14 个 Direct Recipe 固定九节结构 | PASS |
| 14 个“比赛现场直接复制”代码块抽取 | PASS |
| PC GCC 严格编译 | PASS |
| 14/14 Recipe PC 真值 | PASS |
| TI Arm Clang Cortex-M0+ 源码编译 | PASS |
| 旧兼容 API + Level B/C 全量 PC 回归 | 234 PASS / 0 FAIL |
| 28 个 Level C README：复制清单、真实调用、输入、输出、风险 | 28/28 PASS |
| 主仓库文档/API 一致性检查 | 99 README、107 headers、0 errors |
| 开发板验证 | NOT_RUN |

Recipe 原始结果位于 `10_tests/algorithm_recipes/build/direct_recipe_results.json`。验证器从 Markdown 的 `DIRECT_COPY` 区域抽取代码，避免测试另一份手抄副本。
