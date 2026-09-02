# example08 模块 README 汇总与 SysConfig 补充

本目录所有 `.c/.h/.inc` 原为冻结副本。唯一例外是已授权修复的 `signal_tft_st7789.c`：Bresenham `DrawLine` 缺陷会在部分斜率无限循环，修复细节见文末记录；其余模块源文件未修改。

## 1. GPAMP 缓冲：`signal_gpamp*`

从 `gpamp_buffer` README 复制到 `main.c` 的代码：

```c
signal_result_t gpamp_result = SignalGPAMPBuffer_MakeConfig(1.65f, &g_gpamp_config);
```

SysConfig：`Software -> Add -> GPAMP`，实例名 `SIGNAL_GPAMP_BUFFER`，硬件 `GPAMP`；选 `ADC Buffer mode, ADC-assisted chopping`：`PSEL=IN_POS`、`NSEL=INTERNAL_OUTPUT`、Output Pin `DISABLED`、Chopping `ADC_ASSISTED/16KHZ`，PinMux `IN+=PA26`。TI 的内部连接表规定 GPAMP 输出为 ADC1 的 `CH14`，因此本工程由 ADC1 直接采样，不再串接 OPA1。

## 2. OPA0 单位增益缓冲：`signal_opa*`

OPA0 直接缓冲 PA26 的 1.65 V 带偏置信号；若配置成 +4 同相 PGA，1.65 V 会被放大到约 6.6 V 而饱和。因此从通用 OPA README 使用 BUFFER 配置：

```c
static signal_opa_config_t g_pga_config = {
    .mode = SIGNAL_OPA_MODE_BUFFER
};
```

SysConfig 添加 OPA，实例 `SIGNAL_OPA_BUFFER`/`OPA0`：选择 TI `CONFIG_PROFILE_2` 后设 Bandwidth=`HIGH`，即 Output Pin `DISABLED`，`PSEL=IN0_POS`，`NSEL=RTOP`，`MSEL=OPEN`，Gain=`N1_P2`，PinMux `IN0+=PA26`。这是硬件单位增益缓冲；输出由 ADC0 的 `CH13` 内部采样。

## 3. OPA1 的 COMP.DAC8 基准缓冲：`signal_opa*`

从通用 OPA README 复制第二个 BUFFER 软件预算：

```c
static signal_opa_config_t g_dac_buffer_config = {
    .mode = SIGNAL_OPA_MODE_BUFFER
};
```

SysConfig 添加另一 OPA，实例 `SIGNAL_OPA_DAC_BUFFER`/`OPA1`：Output Pin `DISABLED`，`PSEL=DAC8_OUT`，`NSEL=RTOP`，`MSEL=OPEN`，Gain=`N1_P2`，Bandwidth=`HIGH`。它缓冲 COMP0 的 DAC8 基准；由于 PA16 由 SPI1 MISO 占用，本版本不把该输出引到引脚。

`signal_opa_inverting*` 与 `signal_opa_noninverting_pga*` 保持为比赛模块副本但不启用：实测诊断表明旧的反相级输出已经饱和，改为 TI 已定义的 Buffer Quick Profile 后才可得到可靠双路采样。模块的 `.c/.h` 没有修改。

## 4. 比较器：`signal_comparator*`

从两个 README 复制的配置计算：

```c
signal_result_t zero_result = SignalComparatorZeroCross_MakeConfig(
    1.65f, 0.02f, &g_zero_cross_config);
signal_result_t threshold_result = SignalComparatorThreshold_MakeConfig(
    2.00f, 0.02f, false, &g_threshold_config);
```

SysConfig：添加 `COMP0`，实例名 `SIGNAL_COMP_ZERO_CROSS`；Enable Channel=`POS`、Mode=`ULP`、Hysteresis=`20 mV`、Reference Source=`VDDA DAC`、Reference Terminal=`NEG`、DAC Control=`SW`、DAC Code=`128`，使能 `OUTPUT_EDGE` 和 `OUTPUT_EDGE_INV` 中断，PinMux `IN0+=PA26`。`PA26` 同时是 GPAMP 高阻输入，按 TI `gpamp_buffer_to_adc` 示例使用 `assignAllowConflicts` 和 `scripting.suppress` 显式说明共享。

因原比较器 README 没有“运行时切 DAC 码”的代码，补充并复制到 `main.c` 的桥接代码如下；它是唯一必要的硬件控制小逻辑：

```c
DL_COMP_setDACCode0(SIGNAL_COMP_ZERO_CROSS_INST, dac_code);
```

`COMP0_IRQHandler` 的 `DL_COMP_getPendingInterrupt` 分发框架来自 TI 官方 `comp_lp_dac_vref_internal` 示例。

## 5. 双 ADC/DMA、显示、键盘和相位

沿用原有模块 README：ADC0 Mem0=`CH13 (OPA0 output)`、ADC1 Mem0=`CH14 (GPAMP output)`；TIMG0 的两个 event publisher 同时触发 ADC/DMA。根据 10 kHz 实测显示需要，Timer 改为 2 us，即采样率 500 kS/s、每帧 512 点；这与 `signal_config.h` 的 `SIGNAL_SAMPLE_RATE_HZ` 保持一致。ST7789 和矩阵键盘引脚不改。相位模块只处理两组同步 DMA 数据，不配置硬件。

每次修改 SysConfig 后：保存/Generate → CCS Refresh → Clean/Build；只核对生成宏，不修改生成文件。
# ST7789 模块缺陷修复记录

2026-08-20 经实板和斜率遍历验证，`signal_tft_st7789.c` 的 `TFT_ST7789_DrawLine()` 原 Bresenham 实现错误地在更新 `err` 后再次用它判断第二个步进条件；部分斜率会越过终点并无限循环。已改为每轮保存 `e2 = 2 * err`，两次判断均使用该快照。此修复同步自 `MSPM0_Signal_Contest/12_external_devices/display/st7789` 集成库；公开 API 和其余模块文件不变。
