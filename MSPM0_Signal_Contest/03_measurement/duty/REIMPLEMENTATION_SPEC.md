# Duty clean reimplementation specification

状态：`SPECIFICATION_COMPLETE`  
重建类型：`SOURCE_LOST → CLEAN_REIMPLEMENTATION`  
重要声明：本实现不是原源码恢复，`REIMPLEMENTED != RESTORED`。

## 1. Purpose

从一帧双状态模拟波形 `samples[N]` 中，按中间参考电平寻找上升沿和下降沿，计算完整周期内的高电平脉宽、低电平脉宽、周期、频率和占空比。目标是比“统计高于阈值的离散采样点比例”获得更好的亚采样点时间精度，并允许对多个完整周期累计。

本 Primitive 只负责 duty/period/high-width/low-width。`pulse_timing` Recipe 中的 10/90 或 20/80 上升/下降时间仍属于另一个多阈值问题。

## 2. Standards and algorithm references

1. [IEEE 181-2025, Standard for Transitions, Pulses, and Related Waveforms](https://standards.ieee.org/ieee/181/10551/)：脉冲状态电平、参考电平和波形参数算法的标准框架。
2. [NIST: The IEEE Standard on Transitions, Pulses, and Related Waveforms, Std-181](https://www.nist.gov/publications/ieee-standard-transitions-pulses-and-related-waveforms-std-181)：IEEE 181 的公开背景和算法化测量说明。
3. [NI: Reference and State Levels](https://www.ni.com/docs/en-US/csh?context=lvcore_lvwave_state_and_reference_levels)：两个相邻采样点跨过参考电平时，可用线性插值近似 crossing 的精确时刻；常用参考电平为 10%、50%、90%。
4. [Keysight FlexDCA: Duty Cycle](https://helpfiles.keysight.com/scopes/FlexDCA-UG/Content/Topics/Oscilloscope-Mode/Time-Measurements/duty_cycle.htm)：正脉宽与周期均由中间阈值 crossing 定义，占空比为正脉宽/周期。
5. [Keysight: Identifying Waveforms](https://helpfiles.keysight.com/scopes/FlexDCA-UG/Content/Topics/Configure-Meas/z_identifying_waveforms.htm)：Top/Base 决定参考阈值；正常波形常用 10/50/90，振铃明显时可考虑 20/50/80。

访问日期：2026-08-13。

## 3. Input

```c
const float *samples;
uint32_t count;
float sample_rate_hz;
const signal_duty_config_t *config;
```

- `samples`：只读、连续的 `float` 数组；单位可以是 V，也可以是其他线性幅值单位。
- `count`：样本数，至少为 3；成功测量实际要求帧内存在一个完整的 rising→falling→next rising 周期。
- `sample_rate_hz`：真实采样率，单位 Hz，必须有限且大于 0。
- `config`：阈值、滞回、最小幅度和电平模式。

电平模式：

- `SIGNAL_DUTY_LEVELS_AUTO_MIN_MAX`：从整帧有限样本的 min/max 取得 Base/Top。简单、零工作区，但对孤立毛刺敏感。
- `SIGNAL_DUTY_LEVELS_EXPLICIT`：调用者提供已校准或稳健估计的 `low_level`、`high_level`。低占空比、强过冲、强噪声时优先使用。

## 4. Output

```c
signal_duty_result_t
```

| 字段 | 单位 | 定义 |
|---|---|---|
| `duty_ratio` | 1 | 所有有效完整周期的 `Σhigh_width / Σperiod` |
| `duty_percent` | % | `100 * duty_ratio` |
| `period_s` | s | 有效完整周期的平均周期 |
| `frequency_hz` | Hz | `1 / period_s` |
| `high_width_s` | s | 有效完整周期的平均高电平脉宽 |
| `low_width_s` | s | 有效完整周期的平均低电平脉宽 |
| `low_level` / `high_level` | 输入幅值单位 | 实际使用的 Base/Top |
| `threshold_level` | 输入幅值单位 | 中间参考电平 |
| `valid_cycle_count` | cycles | 参与结果的完整周期数 |
| `rising_edge_count` / `falling_edge_count` | edges | 通过滞回确认的边沿数 |

## 5. Buffer and memory rules

- 输入只读，允许位于静态 RAM、DMA 完成后的缓冲区或只读测试数据。
- 不允许 `samples == NULL`，不允许 `result == NULL`。
- 模块不修改输入，不动态分配内存，不保存跨调用状态。
- 算法使用 O(1) 局部状态；除结果结构外不需要 caller workspace。
- 失败时 `result` 保持调用前内容不变，避免消费者误用半成品。

## 6. Algorithm definition

1. 验证所有参数和全部输入样本均为有限数。
2. 根据配置取得 `low_level`、`high_level`，并检查幅度大于 `min_amplitude`。
3. 计算：

   ```text
   threshold = low + threshold_ratio * (high - low)
   lower_guard = low + (threshold_ratio - hysteresis_ratio) * (high - low)
   upper_guard = low + (threshold_ratio + hysteresis_ratio) * (high - low)
   ```

4. 状态机只在波形到达 `lower_guard`/`upper_guard` 后确认 Low/High 状态，防止中阈值附近噪声造成重复边沿。
5. 相邻样本 `y0,y1` 跨越中阈值时，crossing 的小数采样位置为：

   ```text
   crossing = i0 + (threshold - y0) / (y1 - y0)
   ```

6. 严格配对 `rise_i → fall_i → rise_(i+1)`。只有完整且 `0 < high_width < period` 的周期进入累计。
7. 按时间累计：

   ```text
   duty_ratio = sum(high_width_samples) / sum(period_samples)
   ```

   这与“时间区间内脉冲持续时间之和/总时间”的 duty-factor 定义一致；对周期轻微抖动也不会让短周期和长周期拥有不合理的相同权重。

## 7. Edge cases and error behavior

| 情况 | 返回值 |
|---|---|
| 空指针、非法枚举、非法配置 | `SIGNAL_ALGORITHM_INVALID_ARGUMENT` |
| `count < 3` | `SIGNAL_ALGORITHM_INSUFFICIENT_DATA` |
| 样本、采样率或配置中出现 NaN/Inf | `SIGNAL_ALGORITHM_NUMERIC_ERROR` |
| 显式 `high_level <= low_level`、阈值/滞回越界 | `SIGNAL_ALGORITHM_OUT_OF_RANGE` |
| 自动电平得到常量信号，或幅度不超过 `min_amplitude` | `SIGNAL_ALGORITHM_NO_FEATURE` |
| 找不到至少一个完整 rise→fall→rise 周期 | `SIGNAL_ALGORITHM_NO_FEATURE` |
| 计算结果溢出或非有限 | `SIGNAL_ALGORITHM_NUMERIC_ERROR` |

帧开头或结尾的残缺周期被忽略，不当作错误周期参与平均。

## 8. Precision expectations

- 无噪声、边沿在采样间线性变化且 Base/Top 正确时，线性插值应消除单纯整数 sample crossing 带来的量化误差。
- C `float` 与 Python double reference 的有效结果：绝对误差目标 `<= 2e-5`（ratio 和以 sample 表示的内部时刻折算结果）。
- 合成无噪声 trapezoid 扫描：占空比绝对误差目标 `<= 2e-4`。
- 噪声下精度取决于 SNR、边沿斜率、电平估计和周期数；不承诺固定仪器级误差。需要时使用显式校准电平、Hampel/Median 预处理和更多完整周期。

## 9. Dependencies

- `signal_algorithm_status.h`
- C 标准库：`math.h`、`stddef.h`、`stdint.h`
- 无 SysConfig、CMSIS-DSP、IQMath、动态内存或 MCU 寄存器依赖。

## 10. Old API compatibility

历史证据只确认两个符号名：

- `SignalDuty_F32`
- `SignalDuty_GetModuleStatus`

旧头文件参数类型和返回契约已经丢失；对象文件不得用于反编译实现。因此不能安全提供同名二进制/源码兼容包装。

明确映射：

| 旧 API | 新 API/处理 | 兼容性 |
|---|---|---|
| `SignalDuty_F32` | `SignalDuty_Process` | 功能替代，但签名未知，**不是 drop-in compatible** |
| `SignalDuty_GetModuleStatus` | 读取 `MODULE_CARD.yaml`/`VERIFICATION.yaml` | 删除运行时状态查询；验证状态属于证据元数据 |

## 11. Verification reset

```yaml
historical_verification: BUILD_VERIFIED
current_verification: DRAFT
```

只有新的 Python reference、C/Python 一致性、PC 边界测试、TI Arm Clang compile 和完整 target link 分别通过后，才能逐级更新当前状态。历史状态不参与升级。
