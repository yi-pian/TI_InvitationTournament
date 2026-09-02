# 05 幅频/相频扫频仪使用与复用说明

## 1. 工程作用

本工程主动输出一组扫频正弦，在每个频点同步测量 CH1 输入和 CH2 输出，保存并显示增益与相位曲线。

适合测量：

- 低通、高通、带通和陷波网络；
- 放大器闭环频率响应；
- 移相网络和补偿网络；
- 赛题中的幅频、相频和截止频率趋势。

## 2. 接线

```text
DAC0/PA15 ──> 被测网络输入
              ├──> CH1：测实际输入
被测网络输出 └──> CH2：测实际输出
开发板、信号地和被测网络必须共地
```

| 参数 | 默认值 |
|---|---:|
| Start | 200 Hz |
| Stop | 10000 Hz |
| Points | 16 |
| Vpp | 1.0 V |
| 最大点数 | 32 |
| ADC 请求采样率 | 100 kSa/s |

## 3. 页面与按键

| 按键 | 功能 |
|---|---|
| A/B | 上一/下一页面 |
| C | 选择 Start、Stop、Points、Vpp |
| `*` / `#` | 减小/增大当前参数 |
| D | START、STOP；HOLD 后按 D 重新开始 |

页面：

- `GAIN`：幅频曲线；
- `PHASE`：相频曲线；
- `CURRENT`：当前频率、Gain ratio、Gain dB、Phase 和点号。

## 4. 扫频状态与数据流

```text
STOP
  -> D：生成频点表，index=0，进入 RUN
  -> 设置当前正弦频率和 Vpp
  -> 等待输出和被测网络稳定
  -> 同步采集 CH1/CH2 一帧
  -> 两路各计算一次 Vpp
  -> Gain = CH2 Vpp / CH1 Vpp
  -> 双通道相位模块计算 Phase
  -> 保存 frequency/gain_db/phase_deg
  -> index++，局部更新曲线或数字
  -> 最后一点后进入 HOLD
```

每个频点只采集一帧。曲线页面直接读取已经保存的数组，不重新测量或重新计算 FFT。

## 5. 复用的 fuyong 与 example 内容

| 来源 | 当前函数/能力 | 类型 | 用途 |
|---|---|---|---|
| `example02` / Frequency Sweep | `GenerateSweepTable()` | `FUYONG_COPY` | 生成扫频频点 |
| `90_dds_usage` | `SignalWaveOutput_SineWithOffset()` | `FUYONG_ADAPTED` | 主动输出每个频点 |
| `04_dual_adc_dma` | `AcquireDualADCFrame()` | `FUYONG_COPY` | 同步采集输入/输出 |
| `30_basic_measurement` | `MeasureVpp()` | `FUYONG_ADAPTED` | 参数化两路幅值 |
| `40_dual_channel_measurement` | Dual ADC Phase 调用 | `FUYONG_ADAPTED` | 相频结果 |
| `example02` | 单点扫频流程 | `FUYONG_ADAPTED` | `RunSweepPoint()` |
| 本工程 | START/STOP/HOLD、参数限幅、三页面 | `READY_PROJECT_LOCAL` | 完整扫频仪 |
| `70_keypad_usage`、`moni01` | 8 项按键队列 | `FUYONG_ADAPTED` | 可靠交互 |
| `80_tft_usage`、`moni01` | 静态框架 + 曲线/数字局部刷新 | `FUYONG_ADAPTED` | 减少 TFT 传输 |

## 6. 如何使用

1. 按接线图连接 DAC、CH1、CH2 和公共地。
2. Build 并烧录，先停留在 STOP 状态设置参数。
3. 用 C 选参数，用 `*`/`#` 调整。
4. 按 D 启动扫频。
5. 在 CURRENT 页确认每个频点的输入幅值和结果是否合理。
6. 扫频完成进入 HOLD 后，在 GAIN/PHASE 页查看曲线。
7. 再按 D 从第一个频点重新测量。

被测网络的建立时间若明显长于当前固定等待时间，需要在 `RunSweepPoint()` 中按电路特性增加 settle 时间。过短会使低频或高 Q 网络的测量失真。

## 7. 如何复用到其他工程

### 7.1 复用最小扫频状态机

复制：

- `sweep_state_t`；
- `GenerateSweepTable()`；
- `RunSweepPoint()`；
- frequency/gain/phase 静态数组；
- `sweep_index` 和 `sweep_points`。

目标工程必须同时具备 Wave Output、双 ADC、相位模块及对应 SysConfig。

### 7.2 只复用幅频，不要相频

保留输出、采集、两路 Vpp 和 Gain 计算，删除相位模块调用与 phase 数组。不要为了只测幅频而重新设计 DDS 或 ADC 模块。

### 7.3 复用曲线显示

复制参数化 `DrawCurve()`。调用前只清图框内部，外框和标签保留。数组必须使用文件作用域 static，不能将 32 点或更大的曲线数组放在函数栈中。

### 7.4 更换频点策略

优先修改 Frequency Sweep 配置中的线性/对数选项或使用已验证模块支持的模式。不要在主循环中临时写未经验证的频率递推公式。

### 7.5 组合 THD 测量

若每个频点还需 THD，可在采集后对目标通道执行一次 FFT，再让频率、THD 和频谱共享该 magnitude。注意额外 FFT 会增加每点时间和 SRAM 工作区需求。

## 8. 验证状态

- Generate / Compile / Link：PASS；
- `-Wall -Werror`：PASS；
- Flash / Board：NOT_RUN；
- SRAM：8026 B（24.49%）；
- Flash：42592 B（32.50%）。

实板验收需要使用已知截止频率的 RC 网络，确认 Gain 方向、相位正负号、频率设置和实际 DAC 输出。
