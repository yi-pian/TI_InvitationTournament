# Peak Detection：时域峰值与频谱峰值

“找峰”至少分两类：频谱区间最大值，以及时域局部峰/谷。新工程的频谱区间最大值优先使用 `recipe_peak_detect`（CMSIS Min/Max + 应用层 bin 范围）；旧 `SignalPeakDetect_Process` 仅在 legacy compatibility 区维护，不用于新工程。

## 输入与输出

- 频谱输入：非负 `magnitude[bins]`；输出主峰 bin、幅值和可选 fractional bin/frequency。
- 时域输入：`voltage_v[N]`、`Fs`；输出局部峰/谷的时间、幅值、prominence、峰间距和有效峰数。

## 频谱峰逻辑链

```text
RemoveDC → Window → FFT → Magnitude
→ 限定频率范围并排除DC
→ PeakDetect找整数主峰
→ 检查左右邻点/局部最大 → Parabolic或Log-Parabolic插值
→ fractional_bin*Fs/N → frequency
→ coherent gain或多bin能量修正幅值
```

限制范围防止 DC、镜像或无关高频噪声被选中；插值只改善孤立主瓣的峰顶估计，不会把两条重叠谱线分开。

## 为什么频谱峰和时域峰必须分开

频谱数组的索引代表频率 bin，目标通常是某个搜索带内的能量最大值；时域数组的索引代表时间，目标可能是一串有最小间距和 prominence 的局部峰。两者的边界、噪声和输出列表完全不同，不能把现有频谱 `PeakDetect` 直接用于振铃峰列。

## 时域局部峰逻辑链

```text
保留真实瞬态的原始/轻度限带波形
→ 估计基线与噪声尺度（Median/MAD）
→ 三点局部极大/极小候选
→ amplitude/prominence阈值
→ 最小峰间距抑制同一振铃峰重复触发
→ 可选三点抛物线时间/幅值插值
→ 输出峰列表与质量标志
```

局部峰候选的十几行循环可先放 Recipe；但 prominence、最小距离、边界和容量处理会被振铃/包络/触发反复使用，因此可能升级为正式 Primitive。

## 推荐与失效

| 目标 | 默认 | 备选 | 失效条件 |
|---|---|---|---|
| 单个主频 | 频谱范围最大+插值 | 多 bin 质心 | 多个近邻强音、平顶/削顶 |
| 振铃峰序列 | 时域局部峰+prominence+distance | 包络后找峰 | 噪声峰与真峰同量级 |
| 弱已知频率 | LockIn/SineFit | FFT peak | 最大噪声 bin 被误认目标 |

## 采样、抗噪声与精度

- 频谱遵守 FFT N/窗要求；时域峰需要 Fs 和模拟带宽能够重建峰形，建议目标最窄峰至少 5 个点。
- 不要用 Median/Hampel 处理要测的尖峰；可只对基线/多次峰值结果做 MAD 筛选。
- 三点插值在峰顶平滑且 SNR 高时可给亚样本位置；尖顶、平顶、边界峰应拒绝插值。

## MCU / RAM 与 Primitive

- 频谱峰在已有 FFT 后 O(bins)，O(1)；时域峰 O(N)，峰列表 O(P)。
- 复用 PeakDetect、FFT interpolation、MAD、MultiBinEnergy。
- 缺失 `time_peak_list` Primitive：建议 P1，需容量错误、最小间距、prominence、峰/谷、边界与 Python golden tests。

Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 区间频谱主峰；时域局部峰/谷列表 |
| 2 | 输入 | `magnitude[bins]`，或 `voltage_v[N]`+Fs |
| 3 | 输出 | peak bin/value/frequency，或时域 time/value/prominence 列表 |
| 4 | 完整逻辑链 | 见频谱峰与时域峰两条链，不得混用 API |
| 5 | 步骤原因 | 搜索范围、prominence、最小间距和插值分别抑制 DC/噪峰/重复峰/网格误差 |
| 6 | 默认算法 | 频谱区间最大+三点插值；时域三点局部极值+prominence+distance |
| 7 | 可选增强 | 多 bin 质心、log-parabolic、轻度限带/包络 |
| 8 | 适用条件 | 孤立谱峰，或时域峰宽/间隔可分辨 |
| 9 | 不适用条件 | 平顶/削顶、近邻峰未分离、边界峰、噪声峰与真峰等量级 |
| 10 | 采样率 | 时域最窄峰建议至少约 5 点；频谱按目标带宽与抗混叠设置 |
| 11 | 点数/周期数 | FFT N 为 2 次幂；时域记录覆盖基线和全部目标峰 |
| 12 | 抗噪 | MAD 定 prominence；搜索频段/最小间距；不删除真实峰 |
| 13 | 精度增强 | 三点插值、增大 N/记录、幅值 coherent-gain/多 bin 校准 |
| 14 | 计算量 | 已有谱后 O(bins)；时域 O(N) |
| 15 | RAM | 频谱 O(1)；时域峰列表 O(P)，必须传 capacity |
| 16 | Primitive | PeakDetect、FFT interpolation、MAD、MultiBinEnergy；`time_peak_list` 缺失 |
| 17 | 仓库路径 | `04_dsp/peak_detect`、`05_precision/{fft_parabolic_interpolation,log_parabolic_interpolation,multi_bin_energy}` |
| 18 | 伪代码 | `limit range -> candidate -> quality -> optional interpolation -> report` |
| 19 | MCU 调用 | 见下方真实频谱 API；时域多峰只按 Recipe 写应用逻辑 |
| 20 | 失败排查 | 查搜索范围/DC、局部最大条件、左右邻点、平顶、prominence、capacity 和单位 |

```c
SignalPeakDetect_Process(magnitude, bin_count, start_bin, end_bin, &peak);
SignalFFTParabolicInterpolation_Process(magnitude, bin_count, peak.peak_index,
                                        sample_rate_hz, N, &interpolated);
```
