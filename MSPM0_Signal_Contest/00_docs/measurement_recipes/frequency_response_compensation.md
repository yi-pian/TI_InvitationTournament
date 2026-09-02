# Frequency Response Compensation：测量链频响补偿

## 输入与输出

- 输入：同频点的 `H_measured(f)` 与已知 thru/fixture 响应 `H_fixture(f)`，或幅值 dB/相位表；目标查询频率。
- 输出：补偿后的 DUT gain/phase 或样本幅值，校准区间与表外状态。

## 复数补偿链

```text
用已知直通/标准件完成基准扫频
→ 每点保存 fixture_gain_db、fixture_phase_deg、质量标志
→ phase沿频率unwrap
→ 正式测DUT得到 measured_gain_db/phase
→ 在校准频点之间插值fixture响应
→ corrected_gain_db = measured_gain_db - fixture_gain_db
→ corrected_phase   = unwrap(measured_phase) - fixture_phase
→ 最后按显示需要wrap相位
```

等价复数关系是 `H_dut=H_measured/H_fixture`。在 dB/相位形式下做相减更适合 MCU；但插值前相位必须 unwrap，否则 179° 到 -179° 会错误穿过 0°。

## 为什么每一步存在

thru 基准提供“测量夹具自身”的幅相；unwrap 防止相位跨 ±180° 时插值走错方向；表内插值把稀疏校准点映射到实际测试频率；复数相除或 dB/phase 相减才会把 fixture 从测量值中移除。任何一步缺失都会把前端响应继续算进 DUT。

## 推荐、备选与失效

| 情况 | 默认 | 备选 | 失效条件 |
|---|---|---|---|
| 扫频点与校准点相同 | 逐点复数相除/dB相减 | 无 | fixture 幅值接近噪声底 |
| 查询位于表内 | log-frequency 上线性插值 dB/unwrap phase | 更密表 | 表外外推 |
| 只补幅值 | gain dB LUT | 线性 gain LUT | 后续还要相位却未保存 |
| 实时波形逆滤波 | 经离线设计的 FIR/IIR | 频域补偿 | 直接逐 bin 除以接近零的响应导致噪声爆炸 |

## 采样、校准与精度

- 基准和 DUT 使用同一线缆、量程、Fs、N、窗口、负载和连接方式。
- 每个频点多帧 Median/MAD；对低质量校准点不插值穿越，应补测。
- 校准间距在响应变化快处加密；对数频率更适合跨 decade。
- 系统时延产生线性相位斜率，可由 Multi-channel Delay Compensation 先处理，也可作为 fixture phase 一部分保留，但定义要一致。

## MCU / RAM 与 Primitive

- 点结果补偿 O(K)，表内查询 O(log K)；每点保存 `frequency/gain/phase` 约 12 字节，另加质量字段。
- FIR/IIR 仅用于已离线验证的时域逆滤波，不应比赛现场凭曲线自动生成不稳定 IIR。
- 正式 [`frequency_response_correction`](../../05_precision/frequency_response_correction/README.md) 已提供 gain/phase correction LUT、linear/log-frequency interpolation、跨 ±180° 的最短相位插值和越界 reject/clamp。它不负责生成标定表，也不等于时域逆滤波；更复杂的 phase unwrap/complex inverse response 仍是缺口。

Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 去除 fixture/测量链的幅频和相频响应 |
| 2 | 输入 | `H_measured(f)`、`H_fixture(f)` 或 gain dB/unwrap phase 表 |
| 3 | 输出 | 补偿 DUT gain/phase、有效区间/质量/表外状态 |
| 4 | 完整逻辑链 | 基准扫频 -> 保存幅相 -> unwrap -> 表内插值 -> dB/phase 相减 |
| 5 | 步骤原因 | 复数相除分离 fixture；相位插值前 unwrap 避免 ±180° 假跳 |
| 6 | 默认算法 | log-frequency 上线性插值 gain dB 和 unwrap phase |
| 7 | 可选增强 | 同频逐点复数相除、加密表、离线验证 FIR 逆滤波 |
| 8 | 适用条件 | 基准与 DUT 同配置/连接，fixture 响应不接近噪声底 |
| 9 | 不适用条件 | 表外外推、校准点低质量、逐 bin 除以近零响应 |
| 10 | 采样率 | 基准和 DUT 每点 Fs/N/窗完全一致 |
| 11 | 点数/周期数 | 每频点按 Frequency Response Recipe；响应陡变处加密表 |
| 12 | 抗噪 | 每点多帧 Median/MAD，低质量点重测而非跨越插值 |
| 13 | 精度增强 | 同一线缆/负载/量程、delay 分离、多温度/量程校准表 |
| 14 | 计算量 | O(K) 或查询 O(log K) |
| 15 | RAM | 每校准点约 12 字节 + quality/metadata |
| 16 | Primitive | Gain/Phase/Delay Calibration；`phase_unwrap`/通用 LUT/complex correction 缺失 |
| 17 | 仓库路径 | 本 Recipe、`frequency_response.md`、`multichannel_delay_compensation.md` |
| 18 | 伪代码 | `calibrate fixture -> unwrap -> interpolate at f -> measured - fixture -> quality` |
| 19 | MCU 调用 | 下方是结果域标量公式，不伪造未存在的 correction API |
| 20 | 失败排查 | 查配置一致性、表内范围、相位 unwrap、fixture 近零、单位 dB/ratio 和 delay 定义 |

```c
/* APPLICATION RECIPE：fixture_* 已由表内插值得到。 */
corrected_gain_db = measured_gain_db - fixture_gain_db;
corrected_phase_deg = wrap_phase_deg(measured_phase_unwrapped_deg - fixture_phase_deg);
```
