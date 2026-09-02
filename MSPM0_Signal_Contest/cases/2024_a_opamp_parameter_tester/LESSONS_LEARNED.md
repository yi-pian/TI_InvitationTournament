# Lessons Learned

只收录有最终文件、已有 `24A_Q2_Q4_BEGINNER_GUIDE.md` 或本开发会话佐证的问题。没有 FFT 路径，所以本案例没有 FFT Bug 经验。

## L01：新导入工程缺少 `device.opt`

- symptom：Clean 成功，但编译 `main_template.c` 和 `peripheral_system_template.c` 时出现 `no such file or directory: '@device.opt'`。
- root_cause：新工程没有先让 SysConfig Generate 生成 Debug 下的 `device.opt` 和配置源。
- wrong_attempt：把它当普通 include path 或 C 代码错误。
- fix：确保母项目实际包含 `profile.syscfg`，在 CCS/SysConfig Generate 后再 Build。最终 Debug 中存在 `device.opt`、`ti_msp_dl_config.*` 和 `Event.dot`。
- prevention：母项目审查先查 `.syscfg`、构建规则和 Generate 产物；不要从别的工程长期手抄生成文件。
- evidence：开发会话 Build 日志；最终 `Debug/device.opt` 和生成文件。

## L02：AD9850 无输出首先查接线

- symptom：代码已烧录，示波器没有 DDS 输出。
- root_cause：W_CLK/FQ_UD/DATA/RESET 或输出路径接线错误；用户确认当时是线接错。
- wrong_attempt：继续怀疑频率字和初始化代码。
- fix：按模块资料逐线核对电源、地、串行模式、四根控制线和实际测量输出，纠正接线后输出恢复。
- prevention：陌生外设 Bring-up 按“电源/地 → 模式 → Reset → 总线波形 → 输出脚”顺序；代码正确与板上连线正确分开证明。
- evidence：开发会话用户确认；最终 AD9850 驱动与 GPIO SysConfig。

## L03：DAC code 2048 只有约 0.1 V

- symptom：PA15 没有接近中点电压，只有 0.1 V 量级。
- root_cause：SysConfig 的 DAC output amplifier 未打开，输出驱动状态不符合当前负载路径。
- fix：在 DAC0 SysConfig 中启用 output pin 和 amplifier。用户随后实测约 1.647 V。
- prevention：DAC Bring-up 不只查数字码，还要查参考、输出使能、buffer/amplifier、负载和测点。
- evidence：`profile.syscfg` 中 `dacOutputPinEn=true`、`dacAmplifier="ON"`；开发会话实测。

## L04：POWER_ADC 与 UART 报 SysConfig 名称冲突

- symptom：第一次 CLI 验证失败，POWER_ADC 的内部 GPIO config object 与现有 UART TX 同名 `ti_driverlib_gpio_GPIOPinGeneric1`。
- root_cause：不是 PA17 与 UART 物理 Pin 冲突，而是两个 SysConfig 配置对象名字重复。
- wrong_attempt：按 Pin 冲突去移动 PA17 或 UART。
- fix：把 `ADC122.adcPin2Config.$name` 改为未使用的 `ti_driverlib_gpio_GPIOPinGeneric8`；重新验证 0 error、0 warning。
- prevention：同时检查物理 Pin/instance owner 和 SysConfig 内部对象名；不要把两类冲突混为一谈。
- evidence：历史教程 4.5；当前 `profile.syscfg`。

## L05：名义 3.555556 MSPS 的 SR 时间轴不准

- symptom：示波器在 ADC 实际输入点测得 20%～80% 约 24.9 us，MCU 显示约 14.81 us；幅度约 9.38 V vs 9.59 V，主要误差在时间。
- root_cause：程序把 Timer 配置触发率直接当作 ADC/DMA 实际保留样点率。当前整链在该目标速率下没有形成可信时间基准；不能据此断言 ADC 芯片极限只有 2 MSPS。
- wrong_attempt：只改 SR 公式、阈值或输出幅度比例。
- fix：Q3 改为 32 MHz/16 的名义 2 MSPS，时间换算使用模块返回的配置触发率，再与示波器交叉验证。
- prevention：凡是测时间/频率，先用已知波形或示波器验证采样时间轴；`GetConfiguredTriggerRate` 只证明定时器配置，不证明无丢触发。
- evidence：历史教程 6.5、Mode3 源码、用户修改后确认“现在准了”。

## L06：Q2 用 Vpp 把带宽测高

- symptom：实际 ADC 测点的示波器截止频率约 970 kHz，程序用峰峰值路线显示约 1.3 MHz；停止频点和门限幅度也不符合 0.707 预期。
- root_cause：高频小信号的普通 max/min 对噪声和极端码敏感；1%/99% 分位数仍可能受持续扰动和波形采样分布影响，把交流幅值估高。
- wrong_attempt：普通 Vpp；随后尝试 Robust Vpp 仍未满足实测。
- fix：回到去直流 AC RMS，每频点三帧平均，以低频参考的 0.707 判定。
- prevention：带偏置正弦优先使用全样本能量/拟合型幅度；任何算法都必须在真实 ADC 测点和示波器同点比较。
- evidence：历史教程 5.4、9.1、9.2；最终 `App_MeasureACRMS`。

## L07：把 Q3 固定在 100 kHz 无法测慢 SR

- symptom：对低压摆率 DUT，没有清楚的高低平台，阈值交点不代表完整边沿。
- root_cause：100 kHz 半周期 5 us，小于 0.1 V/us、约 9.6 Vpp 条件下 20%～80% 所需约 57.6 us。
- wrong_attempt：认为压摆率测量只要固定一个任意较高频率即可。
- fix：降到 5 kHz，使半周期 100 us，并在一帧内对多条完整边沿分别平均。
- prevention：先用 `t_edge≈threshold_span/SR_min` 反推方波半周期；再用示波器确认高低平台。
- evidence：历史教程 9.3；最终 `MODE3_TEST_FREQUENCY_HZ=5000`。

## L08：用带 1.65 V 偏置信号的均值判断 `-3 dB`

- symptom：改变正弦幅度时，平均值仍接近 1.65 V，无法可靠指示增益下降。
- root_cause：均值表示 DC bias，不是交流正弦幅度。
- wrong_attempt：比较输入/输出原始均值的 0.707。
- fix：AC RMS 内部先求均值再减均值；平均值只作为偏置监视量显示。
- prevention：每个标量都写明物理语义和单位；DC、RMS、峰值、Vpp 不得混用。
- evidence：原始开发方案、历史教程第 1 节、最终 `SignalACRMS_Process` 调用。

## L09：历史教程参数与最终源码不一致

- symptom：教程部分文字仍写 20 mV、10 kHz step，而最终源码是 80 mV、25 kHz step；DDS 注释说约 1 V，但常量是 0.35 V。
- root_cause：调试期间参数多次修改，说明文档没有全部同步。
- fix：本案例以最终 `main_template.c` 指纹和常量为 Build truth，并把差异列为限制。
- prevention：最终验收时对 README/Card/源码参数做自动一致性检查；历史案例永远低于当前源码和当前文档真源。
- evidence：当前 `main_template.c` 与历史教程对照。

## L10：大 Buffer 让 Build 成功但 RAM 余量有限

- symptom：源代码看起来只有几个数组，但最终 SRAM 已用 26,900 B，只剩 5,868 B，配置栈 512 B。
- root_cause：两个 3,072 点 float 级缓冲区各占 12,288 B；MSPM0G3507 无硬件 FPU，浮点计算还需要考虑执行时间和栈。
- fix：历史工程用 union 让 raw 与 Robust workspace 分时共享 12,288 B；Build 后查 Map。
- prevention：组合模块前计算 peak live RAM，不只把每个模块的静态估算相加；只有生命周期不重叠才能复用。
- evidence：最终 Map 中 `g_wave_capture`、`g_wave_voltage` 和 Memory Configuration。
