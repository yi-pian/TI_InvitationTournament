# 测量逻辑链 Recipe 使用说明

按测量目标查找时，从上级 [Measurement Recipe Index](../MEASUREMENT_RECIPE_INDEX.md) 和 [Algorithm Decision Tree](../ALGORITHM_DECISION_TREE.md) 开始。

本目录回答的不是“某个函数怎么调用”，而是“ADC 原始数据怎样一步步变成可信的物理测量结果”。每条 Recipe 都把算法顺序、每一步存在的原因、采样条件、失效条件和校准边界写清楚。

## 三层边界

| 层 | 定义 | 本仓库中的形式 |
|---|---|---|
| Primitive | 输入输出清晰、会反复复用、容易独立测试的基础算法 | 正式 `.c/.h/README`；复杂算法必须有 PC 测试 |
| Recipe | 多个 Primitive 与少量公式组成一个测量逻辑链 | 本目录 Markdown；十几行的一次性公式留在 Recipe |
| Application | 采集、Recipe、控制、显示/通信组合成完整功能 | `MSPM0_Signal_Contest/08_applications/` |

Recipe 不拥有 ADC、Timer、DMA、DAC、GPIO 或 SysConfig。它只接收已经完成的帧和元信息，例如 `raw[]`、`count`、`sample_rate_hz`、VREF、量程、通道时延与校准参数。

## 共同输入契约

- `raw[]`：ADC code；只有 ADC 转电压和原始码量程判断可直接使用。
- `voltage_v[]`：经名义换算、必要时经增益/偏置校准后的物理电压。
- `count=N`：有效点数，不是字节数。
- `sample_rate_hz=Fs`：每通道真实或配置采样率；时间、频率、相位都依赖它。
- 双通道数据必须说明是否同步、是否交错、B 相对 A 的固定采样延迟。

## 所有 Recipe 都要先过的门

1. 检查帧已完成，DMA 不会在处理期间覆盖数据。
2. 检查 ADC 削顶、模拟前端饱和和异常跳码；削顶数据不能靠后续算法“修好”。
3. 明确物理量是否包含 DC、是否要保留尖峰/过冲/谐波；这决定能否 RemoveDC、Median、Hampel 或低通。
4. 先用简单方法得到可解释结果，再按 Recipe 的精度增强项逐级增加复杂度。
5. MCU 资源结论最终以 TI Arm Clang `.map` 和运行时间实测为准；文中的复杂度和 RAM 是算法级预算。

## 精度增强工具箱

| 手段 | 真正能改善什么 | 不能解决什么 |
|---|---|---|
| crossing 线性插值 | 阈值附近近似直线时，把整数采样点时间细化为小数样本位置 | Fs 太低、边沿带宽不足、阈值附近振铃/非单调 |
| 多周期/多边沿平均 | 降低随机时间抖动，观测时间越长频率平均越稳 | 频率在帧内变化；固定采样时钟误差 |
| Median/MAD/Hampel | 识别少量孤立异常值或异常测量结果 | 真实脉冲、过冲、burst；系统性偏差 |
| 相干采样 | 让周期边界与记录对齐，降低谱泄漏并改善单 bin 幅值 | 时钟不共源或频率未知时通常无法严格保证 |
| Window + coherent gain | 非相干 FFT 时抑制泄漏；把窗后的单边谱恢复到幅值标度 | 主瓣内 off-bin scalloping、前端频响误差 |
| ADC/DAC/前端校准 | 消除可重复的增益、偏置、频响和通道时延误差 | 噪声、漂移、饱和、未覆盖温度条件 |
| LUT + interpolation | 用少量标定点近似单调非线性或频响曲线，MCU 计算轻 | 表外外推、迟滞、多值关系、标定点错误 |

## 验证状态

本目录是新的逻辑链知识库，Recipe 整体状态均为 `DRAFT`，直到有对应 PC 端端到端测试或 Application/板级证据。引用的现有 Primitive 保持各自 `PC_VERIFIED` 或 `DRAFT` 状态；Recipe 文档不会把 Primitive 的 PC 验证升级成板级验证。
