# main.c 教学函数化重构报告

## 结论

本轮将 `fuyong` 16 个主题工程的 COPY 单位从 while 循环中的零散语句改为 `main.c` 内完整、可复制的 `static` 函数；没有新增任何 `.c/.h` 模块层，也没有修改已有模块源码。

完整可恢复备份已建立：`fuyong_before_main_function_refactor/`（888 个文件）和 `backup_before_main_function_refactor/`（2246 个文件）。

**Existing module source modified: NO**

## fuyong main.c 变化

| 工程 | main.c 行数（前→后） | static 函数（前→后） | 主要教学函数 |
|---|---:|---:|---|
| 01_adc_basic | 9 → 41 | 0 → 1 | `ReadADCOnce()` |
| 02_adc_dma | 53 → 72 | 0 → 3 | `InitADC()`、`AcquireADCFrame()` |
| 04_dual_adc_dma | 77 → 72 | 0 → 3 | `InitDualADC()`、`AcquireDualADCFrame()` |
| 10_timer_frequency | 50 → 65 | 0 → 2 | `InitTimerFrequencyMeasurement()`、`MeasureTimerFrequency()` |
| 11_zero_cross_frequency | 71 → 238 | 0 → 3 | `AcquireADCFrame()`、`PrepareSignal()`、`MeasureFrequencyZeroCross()` |
| 20_fft_analysis | 163 → 378 | 2 → 10 | `PrepareSignal()`、`RunFFTCommon()`、`MeasureFFTFrequency()`、`RefineFFTFrequency()`、`AnalyzeHarmonicsAndTHD()`、`AnalyzeSNRAndSFDR()` |
| 21_time_domain_waveform | 27 → 124 | 0 → 3 | `AcquireADCFrame()`、`PrepareDisplaySamples()`、`DrawTimeDomainWaveform()` |
| 30_basic_measurement | 49 → 154 | 1 → 4 | `ConvertADCToVoltage()`、`MeasureBasicParameters()` |
| 40_dual_channel_measurement | 37 → 114 | 0 → 3 | `AcquireDualADCFrame()`、`MeasurePhase()`、`CalculateDelayFromPhase()` |
| 50_robust_measurement | 53 → 204 | 0 → 6 | `ConvertADCToVoltage()`、`ApplySelectedFilter()`、`AnalyzeRobustStatistics()` |
| 60_precision_measurement | 27 → 137 | 0 → 4 | `ConvertADCToVoltage()`、`RunSineFit3Param()`、`RunSineFit4Param()` |
| 61_lock_in | 17 → 105 | 0 → 3 | `ConvertADCToVoltage()`、`RunLockIn()` |
| 70_keypad_usage | 24 → 106 | 0 → 4 | `ReadKeypad()`、`HandlePageSwitch()`、`HandleNumberInput()`、`HandleParameterAdjust()` |
| 80_tft_usage | 24 → 114 | 0 → 5 | `InitTFTDemo()`、`DrawStaticText()`、`UpdateLiveValue()`、`DrawPage()` |
| 90_dds_usage | 24 → 93 | 1 → 3 | `InitDDSOutput()`、`SetDDSFrequency()`、`HandleDDSFrequencyAdjust()` |
| 91_dac_usage | 11 → 49 | 0 → 2 | `SetDACDC()`、`ExplainContinuousWaveformEntry()` |

关键函数均增加中文函数头，说明功能、输入/输出、单位、使用的全局变量、算法步骤、返回值、复制前置函数和模块依赖。`main()` 仅保留初始化与“采集 → 准备 → 测量/分析 → 显示”的流程。

## 变量语义审计

发现并修复一项 `SEMANTIC_VARIABLE_FIX`：`11_zero_cross_frequency` 原来将 `frequency_hz` 作为 `arm_mean_f32()` 的临时输出。现在使用专用 `mean_v`（V），`frequency_hz` 始终只表示 Hz。公式、阈值、采样率和测频方法未改变。

## FFT 一帧一次

- `20_fft_analysis`：`arm_cfft_q15` 仅出现于 `RunQ15FFT()`，而其唯一调用方为 `RunFFTCommon()`；同一帧 FFT = 1。
- `reuse_test_01_signal_analyzer`：同样仅由 `RunFFTCommon()` 执行 FFT；同一 DMA frame FFT = 1。
- 频率、插值、THD、SNR、SFDR 和频谱绘制只复用 `fft_magnitude[]`，不重新运行 FFT。

## 文档同步

- 16 份主题 README 已增加“推荐复制函数”。
- `fuyong/INDEX.md`、`COPY_BLOCK_INDEX.md` 已从 COPY 名称同步为“函数名 + COPY 区”。
- `SINGLE_FUNCTION_INTERFACE_STANDARD.md` 已增加统一教学函数命名规范。
- `backup/BACKUP_REUSE_MAP.md` 与 example04 的 `EXAMPLE04_REUSE_MAP.md` 已增加 `App_*` ↔ fuyong 函数边界对照。

## Build 结果

工具：SysConfig 1.28.0、TI Arm Clang 5.1.1.LTS、MSPM0 SDK 2.11.0.07。

- fuyong：16/16 SysConfig Generate、`-Wall -Werror` Compile、Link PASS。详细 RAM/Flash 见 `fuyong/BUILD_MATRIX.md`。
- reuse tests：3/3 Generate、Compile、Link PASS。详细结果见 `fuyong/reuse_tests/REUSE_TEST_BUILD_MATRIX.md`。
- backup 综合工程：11/11 复核通过。10 个 CCS `gmake clean all` PASS；example02 因历史 Debug 自动源清单遗漏 `signal_wave_output_mspm0g3507.c`，gmake 仍 FAIL，但完整实际源码的 SysConfig Generate、`-Wall -Werror` Compile、Link PASS。没有手改生成 makefile。
- Board validation：全部 `NOT_RUN`。

## Flash/SRAM 前后

函数化只是结构重构；优化后的最终二进制基本保持不变。所有最新数值都来自 `build_validation/full/*.map`。代表性变化：11 过零 `7,536/15,260 B → 7,456/15,256 B`（Flash/SRAM）；20 FFT `31,512/21,904 B → 31,528/21,908 B`；50 鲁棒 `10,512/17,304 B → 9,352/17,304 B`。全表见 `fuyong/BUILD_MATRIX.md`。

## backup 综合 Example 同步原则

11 个 backup 综合工程原本已经使用 `App_*` 静态函数分离采集、测量、FFT、显示、按键和题目逻辑。本轮不强行用固定教学函数逐字覆盖，以保留动态采样率、两点校准、可选 Window/滤波、捕获回放、扫频、外设协议和页面状态机。仅同步了与 fuyong 函数的职责边界和来源说明；没有改变其执行语句或题目行为。

## 模块完整性

对 `fuyong` 当前版本与函数化前备份中所有可比较的 `modules/*.c/.h` 做 SHA-256：189 个文件，**0 mismatch**。

**Existing module source modified: NO**
