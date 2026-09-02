# 真实复制复用验收报告

## 1. 结论

三项独立复用测试均为 **PASS**：它们各自从空模板副本建立，不互相复制工程；只带入主题 README/COPY 所需的模块、主循环调用和对应 SysConfig。三项均已用 SysConfig 1.28.0 生成并由 TI Arm Clang 5.1.1.LTS 编译、链接通过。

静态工程检查确认三项的 `.syscfg` 元数据、生成初始化名 `SYSCFG_DL_init()` 和输出文件均一致。CCS 的 Debug makefile 仍由 CCS/CCS Theia 首次 GUI Build 自动生成；本次没有手改该生成文件，而是直接调用同版本 TI Arm Clang 完成逐工程 Compile/Link。

复制的模块源码保持原样：对三个测试工程全部 `.c/.h` 共 **66** 个文件计算 SHA-256，并与其主题来源模块逐文件比较，结果为 **66 matched / 0 mismatch / 0 unmapped**。未修改任何已有集成库模块 `.c/.h`。

## 2. 测试 01：ADC DMA + 基础测量 + FFT + TFT

工程：`reuse_test_01_signal_analyzer`，`main.c` 148 行。

- 组合来源：`02_adc_dma` 的 ADC DMA，`30_basic_measurement` 的时域统计，`20_fft_analysis` 的 `FFT_COMMON_HELPER + FFT_COMMON + FFT_FREQUENCY + FFT_PEAK_INTERPOLATION + FFT_HARMONICS + FFT_THD + FFT_SNR_SFDR`，以及 `21_time_domain_waveform`、`80_tft_usage` 的显示调用。
- 复制模块：ADC DMA；Window、窗增益、FFT 抛物线插值、multi-bin energy、harmonic、THD、SNR、SFDR；TFT core/platform/font；CMSIS-DSP 链接库。
- 接口拼接：零变量重命名。统一使用 `adc_samples`、`SAMPLE_COUNT`、`sample_rate_hz`、`voltage_samples`、`centered_samples`、`fft_magnitude` 和 `frequency_hz` 等约定变量。
- 胶水逻辑：仅 4 段应用级连接（启动/等待 DMA、读取采样率、按顺序调用 COPY 数据链、更新 TFT）；没有重新实现算法。每帧 `App_RunQ15FFT()` 只调用一次，其幅度结果被测频、插值、谐波、THD、SNR、SFDR 共同消费。
- SysConfig：采用原 `PROFILE_06_FULL_SIGNAL` 物理配置以同时具备 ADC 和 TFT；仅将 ADC0、ADC DMA、采样 Timer 的逻辑实例名改为 `SIGNAL_ADC`、`SIGNAL_ADC_DMA`、`SIGNAL_SAMPLE_TIMER` 以匹配 `02_adc_dma` 的公开模块接口。未改引脚、DMA 通道或外围设备。
- 构建：Flash 45,600 B，SRAM 19,784 B；Generate / Compile / Link PASS。

## 3. 测试 02：双 ADC DMA + 相位 + TFT

工程：`reuse_test_02_dual_channel`，`main.c` 42 行。

- 组合来源：`04_dual_adc_dma`、`40_dual_channel_measurement`、`80_tft_usage`。
- 复制模块：双 ADC DMA、双通道相位算法、TFT core/platform/font 和必要 status 头文件。
- 接口拼接：零变量重命名；`adc_ch1_samples` / `adc_ch2_samples` 从采集模块直接输入相位模块。`delay_s` 仅按明确公式 `phase_deg / (360 × reference_frequency_hz)` 计算，未伪造为相位模块的输出。
- 胶水逻辑：3 段（双 ADC 帧获取、phase→delay 换算、TFT 刷新），均为 COPY 调用顺序和显示接入。
- SysConfig：直接使用既有 `PROFILE_06_FULL_SIGNAL`，未改实例、引脚或 DMA。
- 构建：Flash 24,568 B，SRAM 5,068 B；Generate / Compile / Link PASS。

## 4. 测试 03：矩阵键盘 + DDS/DAC + TFT

工程：`reuse_test_03_ui_dds`，`main.c` 42 行。

- 组合来源：`70_keypad_usage`、`90_dds_usage`、`80_tft_usage`。
- 复制模块：矩阵键盘；Wave Output、DAC DMA、DDS、波表及正弦/方波/三角/锯齿/任意波文件；`signal_math.h`；TFT core/platform/font。
- 接口拼接：零变量重命名。`frequency_hz` 贯穿按键步进、DDS 输出和 TFT 显示；`*` 减 10 Hz、`#` 加 10 Hz。
- 胶水逻辑：2 段（按键变更后应用频率、TFT 刷新）；算法和 DMA 输出逻辑均来自复制模块。
- SysConfig：直接使用既有 `PROFILE_06_FULL_SIGNAL`，未改实例、引脚或 DMA。
- 构建：Flash 31,032 B，SRAM 2,594 B；Generate / Compile / Link PASS。

## 5. 文档和 COPY 完整性修正

真实测试发现并修正了三类“复制时容易遗漏”的文档信息，均只改 README/COPY 注释，不改模块实现。

1. `20_fft_analysis`：`FFT_COMMON` 还需要前置的 `FFT_COMMON_HELPER`。COPY 映射已明确为前者的必需依赖，并补齐所有算法模块、TFT 可选模块及 `02_adc_dma` 采集模块的文件清单。
2. `80_tft_usage`：README 现明确列出 TFT core、MSPM0 platform、font 和 font data，以及 `signal_status.h`。
3. `90_dds_usage`：README 现明确列出完整 Wave Output 依赖，特别补充了常被遗漏、但被波表和正弦实现直接包含的 `signal_math.h`。

这些修正后，复用测试不再需要从主题外猜测额外源码文件；但用户仍须为所选硬件组合在 SysConfig 中使用兼容的原始配置，并以生成的宏名为准。

## 6. 未做事项与限制

- 未添加 Feature、Core、Analysis Context 或任何新架构。
- 未修改任何已有模块 `.c/.h`，也没有手改 `Debug` 生成文件。
- 未烧录、未连接实板，Flash 与 Board validation 均为 `NOT_RUN`；本报告只证明 SysConfig、编译与链接层面的可复用性。
- 测试 01 的组合需要三个逻辑实例名对齐。这是 ADC 模块 API 与完整硬件 profile 的命名差异，不是硬件资源冲突；实板验证时仍应检查 ADC 触发速率、TFT 和 ADC 同时运行的时序。

详细逐工程状态见 [REUSE_TEST_BUILD_MATRIX.md](REUSE_TEST_BUILD_MATRIX.md)。
