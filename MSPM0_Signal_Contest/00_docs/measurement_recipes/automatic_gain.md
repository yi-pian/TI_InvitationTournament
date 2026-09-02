# Automatic Gain / AGC：自动增益控制

AGC 的目标是把输出幅值维持在目标附近；它不是一次性自动量程，也不能在正式测量帧中途改变增益。

## 输入与输出

- 输入：每帧幅值估计、目标幅值/允许带、当前 VGA/PGA 控制量、控制上下限、建立时间和标定表。
- 输出：新控制量、稳定/调节/饱和状态、最终实际增益和测量质量。

## 控制链

```text
采一帧 → 用AC RMS/LockIn/目标频率幅值估计包络
→ 削顶则立即向低增益方向安全退档
→ error_db=target_db-measured_db
→ deadband内连续M帧？ → LOCKED，冻结控制并采正式测量帧
→ 否则查VGA标定LUT，限制每步最大增益变化
→ 设置控制量 → 等待建立 → 丢弃过渡帧 → 下一次迭代
→ 达到控制/迭代上限仍不满足则返回 SATURATED/NO_LOCK
```

### 每一步为什么存在

- dB 误差使乘性幅值变化变成加性控制，更适合跨大动态范围。
- deadband、连续 M 帧和步长限制防止噪声引起来回振荡。
- VGA 的控制电压到 gain dB 通常不是理想线性，优先使用实测 LUT 反查。
- 正式幅值/THD/频响测量必须在 AGC 锁定并冻结后完成，否则增益变化会调制波形。

## 推荐与备选

| 情况 | 默认 | 备选 | 失效条件 |
|---|---|---|---|
| 慢变单音 | 帧级步进+LUT逆查 | 比例控制 | 建立时间未知、VGA非单调 |
| 快变包络 | 硬件检波/模拟AGC | 更短软件帧 | M0+帧处理延迟追不上 |
| 只需一次选档 | Auto Range Recipe | AGC | 用连续控制增加不必要复杂度 |

## 采样、抗噪声与精度

- 幅值帧至少覆盖 3～10 周期；每次改增益后等待模拟建立并丢弃过渡帧。
- 弱已知单音用 LockIn；一般周期信号用 AC RMS。对幅值估计做短期平均，但不要让控制带宽超过被控链可承受范围。
- VGA 每档/每个控制点需要增益、偏置、频响和温漂校准；LUT 表外不得外推。

## MCU / RAM 与 Primitive

- 每帧测量 O(N)，控制 O(1)，LUT 查找 O(log K) 或 O(K)。
- 复用 AC RMS、LockIn、Clipping Detect、VGA Calibration Recipe、Median/MAD。
- 当前 `08_applications/automatic_gain` 是 Application 参考；纯算法库不复制其平台控制代码。

## 是否升级 Primitive

AGC 状态机依赖硬件建立时间与控制接口，暂留 Application/Recipe。可独立测试且多平台复用后，才把“纯决策核心”升级为 Primitive。Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 把目标频率/交流幅值闭环维持在允许带内 |
| 2 | 输入 | 每帧幅值、目标/带宽、当前控制、上下限、LUT、settling |
| 3 | 输出 | 新控制、LOCKED/ADJUSTING/SATURATED/NO_LOCK、实际增益 |
| 4 | 完整逻辑链 | 见“控制链”；锁定并冻结后才采正式测量帧 |
| 5 | 步骤原因 | 见四条原因；dB error、deadband、LUT 和过渡帧不可省略 |
| 6 | 默认算法 | 帧级 dB 误差 + 标定 LUT 逆查 + deadband/步长限制 |
| 7 | 可选增强 | 比例控制、硬件检波/模拟 AGC、二分控制 |
| 8 | 适用条件 | 输入慢变、控制到 gain 单调、建立时间已知 |
| 9 | 不适用条件 | 快于帧环路、VGA 非单调/表外、THD 正式测量时仍在调增益 |
| 10 | 采样率 | 由幅值 Recipe 决定；控制更新率必须低于测量/模拟链可稳定速率 |
| 11 | 点数/周期数 | 幅值帧至少 3～10 周期；改增益后丢弃过渡帧 |
| 12 | 抗噪 | 幅值短期平均、deadband、连续 M 帧锁定 |
| 13 | 精度增强 | VGA/DAC/ADC/频响校准，锁定后重新测量 |
| 14 | 计算量 | 测量 O(N)，控制/LUT O(1)～O(log K) |
| 15 | RAM | 幅值测量所需帧 + O(K) LUT + O(1) 状态 |
| 16 | Primitive | AC RMS、LockIn、Clipping、MAD；控制状态机留 Application |
| 17 | 仓库路径 | 本 Recipe、`vga_calibration.md`、`05_precision/lock_in` |
| 18 | 伪代码 | `measure -> safe retreat -> error_db -> deadband M frames OR LUT step -> settle -> repeat` |
| 19 | MCU 调用 | 下方仅是控制伪代码，硬件 set API 必须从 exact driver `.h` 获取 |
| 20 | 失败排查 | 查幅值定义、削顶、控制方向/LUT、deadband、settling、步长、上下限和振荡 |

```c
/* APPLICATION PSEUDOCODE：硬件控制函数名必须现场查真实 .h。 */
error_db = target_db - measured_db;
if (fabsf(error_db) <= deadband_db) stable_frames++;
else requested_control = calibrated_inverse_lut(target_db, control_table, table_count);
```
