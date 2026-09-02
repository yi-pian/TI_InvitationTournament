# 06 触发/猝发捕获仪使用与复用说明

## 1. 工程作用

本工程实现类似示波器 SINGLE 的单次触发捕获。ARM 后连续搜索 CH1，在满足触发电平和斜率条件时保存包含预触发数据的窗口，并进入 HOLD 保持显示。

适合：

- 捕获短脉冲、猝发和一次性事件；
- 检查启动瞬态、阶跃响应和过冲；
- 捕获不方便重复观察的边沿信号；
- 作为赛题触发记录功能的应用层模板。

## 2. 默认接口与参数

| 项目 | 默认设置 |
|---|---|
| 触发输入 | ADC CH1 / PA25 |
| CH2 | 同步 DMA 占位，不参与触发分析 |
| 搜索帧 | 512 点 |
| 保存窗口 | 256 点 |
| 请求采样率 | 200 kSa/s |
| 初始触发电平 | ADC code 2048 |
| 初始斜率 | Rising |
| 初始预触发 | 25% |

## 3. 状态机

| 状态 | 含义 |
|---|---|
| ARM | 清除旧捕获语义，准备重新开始 |
| WAITING | 连续采集搜索帧并等待触发 |
| TRIGGERED | 已找到触发位置，准备整理结果 |
| HOLD | 保持捕获波形，等待重新 ARM |

状态变量只描述捕获流程，`current_page` 只描述显示页面，两者不共用。

## 4. 按键

| 按键 | 功能 |
|---|---|
| D | ARM / Re-arm |
| C | Rising / Falling 切换 |
| `*` | 降低触发电平 |
| `#` | 提高触发电平 |
| 1 / 2 / 3 | 预触发比例 25% / 50% / 75% |

屏幕显示状态、斜率、触发电平、Duration、Start、End、Vpp、预触发比例和捕获波形。

## 5. 运行数据流

```text
ARM
  -> WAITING
  -> 获取唯一搜索帧
  -> Trigger_Capture 检查斜率、电平和滞回
  -> 未找到：继续下一搜索帧
  -> 找到：复制预触发 + 后触发窗口
  -> code→V、计算 Vpp 和时间信息
  -> HOLD，局部刷新捕获波形框和数字
```

HOLD 状态不会重新采集，因此屏幕上的波形和数字属于同一次捕获。

## 6. 复用的 fuyong 内容

| 来源 | 当前函数/能力 | 类型 | 用途 |
|---|---|---|---|
| `04_dual_adc_dma` | `AcquireADCFrame()` | `FUYONG_ADAPTED` | 获取 CH1 搜索帧 |
| `23_trigger_capture` | `Trigger_Capture()` | `FUYONG_ADAPTED` | 斜率、滞回、触发点和预触发窗口 |
| `30_basic_measurement` | code→V 与 Vpp 步骤 | `FUYONG_ADAPTED` | 捕获结果数值 |
| `21_time_domain_waveform` | 屏宽抽点 | `FUYONG_ADAPTED` | 捕获波形 |
| 本工程 | ARM/WAITING/TRIGGERED/HOLD | `READY_PROJECT_LOCAL` | SINGLE 应用状态机 |
| `70_keypad_usage`、`moni01` | 队列式按键处理 | `FUYONG_ADAPTED` | ISR 不启动 DMA |
| `80_tft_usage`、`moni01` | 静态页面和局部捕获区刷新 | `FUYONG_ADAPTED` | HOLD 显示 |

## 7. 如何使用

1. 将待捕获信号连接 CH1 并共地。
2. Build、烧录后设置触发电平、斜率和预触发比例。
3. 按 D ARM。
4. WAITING 时产生待捕获事件。
5. 进入 HOLD 后读取波形、Vpp、Start、End 和 Duration。
6. 按 D 重新捕获。

如果一直 WAITING：

- 检查触发电平是否在信号上下限之间；
- 检查选择的上升/下降沿；
- 检查信号幅值是否足以越过滞回区；
- 检查输入范围和公共地。

## 8. 如何复用到其他工程

### 8.1 复用触发算法

复制 `Trigger_Capture()`、触发配置变量、搜索数组和捕获数组。目标工程应原样复制 `signal_trigger_capture` 模块，不要改模块内部算法。

### 8.2 复用 SINGLE 状态机

复制 `capture_state_t`、`ArmCapture()` 和 main 循环中的四状态分派。DMA 采集必须留在主循环，键盘 ISR 只能写入 ARM 请求或事件队列。

### 8.3 调整预触发长度

修改 `pretrigger_percent` 和静态 `CAPTURE_COUNT`，并重新检查 SRAM。预触发只是决定捕获窗口中触发点的位置，不应改变原始搜索帧的时间顺序。

### 8.4 复用捕获显示

复制 `DrawCaptureWaveform()` 和动态数值区。刷新时只清图框内部；HOLD 之外可以清除旧波形，但不要每个搜索帧都刷新 TFT，否则会拖慢 WAITING 状态。

## 9. 验证状态

- Generate / Compile / Link：PASS；
- `-Wall -Werror`：PASS；
- Flash / Board：NOT_RUN；
- SRAM：4639 B（14.16%）；
- Flash：28088 B（21.43%）。

实板应使用信号源的单次 Burst、按键脉冲或函数发生器触发输出验证预触发位置和边沿方向。
