# Canonical 算法内容交付索引

> 当前第一入口是 [SIGNAL_ALGORITHM_COOKBOOK.md](SIGNAL_ALGORITHM_COOKBOOK.md)。2026-08-13 当前正式算法唯一位于 `03_measurement/04_dsp/05_precision`；模块数量与状态由 Canonical Registry 和 [ALGORITHM_LIBRARY_STATUS.md](ALGORITHM_LIBRARY_STATUS.md) 给出，不从下方示意树手工计数。

本交付已经合入唯一知识根 `MSPM0_Signal_Contest/`；算法源码位于 `03_measurement/`、`04_dsp/`、`05_precision/`，工具位于 `tools/algorithm/`。

## 1 完整目录树

```text
MSPM0_Signal_Contest/
├── README.md
├── 00_docs/
│   ├── INTEGRATION_BOUNDARY.md
│   ├── HARDWARE_ALGORITHM_CONTRACT.md
│   ├── ALGORITHM_INTERFACE_STANDARD.md
│   ├── SIGNAL_DATA_MODEL.md
│   ├── MODULE_INTERFACE_MATRIX.md
│   ├── ALGORITHM_SELECTION_GUIDE.md
│   ├── SIGNAL_PROCESSING_PIPELINE_GUIDE.md
│   ├── ALGORITHM_RECIPES.md
│   ├── MEASUREMENT_RECIPE_INDEX.md
│   ├── measurement_recipes/             # 多 Primitive 组合的测量逻辑链
│   ├── ALGORITHM_PITFALLS.md
│   ├── PHASE_METHOD_SELECTION.md
│   ├── FFT_MEMORY_BUDGET.md
│   ├── DSP_RESOURCE_BUDGET.md
│   ├── ALGORITHM_LIBRARY_STATUS.md
│   └── DELIVERY_INDEX.md
├── 03_measurement/
│   ├── common/                         # status、complex 类型
│   ├── adc_to_voltage/                 # implemented
│   ├── mean/                           # implemented
│   ├── statistics/                     # implemented
│   ├── minmax/                         # implemented
│   ├── vpp/                            # implemented
│   ├── rms/                            # implemented
│   ├── ac_rms/                         # implemented
│   ├── frequency_zero_cross/           # implemented
│   └── phase/                          # implemented adapters
├── 04_dsp/
│   ├── remove_dc/                      # implemented
│   ├── clipping_detect/                # implemented
│   ├── moving_average/                 # implemented
│   ├── median_filter/                  # implemented
│   ├── mad/                            # implemented
│   ├── hampel_filter/                  # implemented
│   ├── fir/                            # implemented engine, external taps
│   ├── iir_biquad/                     # implemented SOS engine, external coeffs
│   ├── window/
│   │   ├── rectangular/                # implemented
│   │   ├── hann/                       # implemented
│   │   ├── hamming/                    # implemented
│   │   └── blackman/                   # implemented
│   ├── fft/                            # radix-2 complex in-place + real adapter
│   ├── fft_magnitude/                  # implemented
│   ├── peak_detect/                    # implemented
│   ├── harmonic/                       # implemented BASIC/COMP radius
│   ├── thd/                            # implemented
│   ├── snr/                            # implemented
│   ├── sfdr/                           # implemented
│   ├── correlation/                    # implemented
│   ├── autocorrelation/                # implemented
│   ├── czt/                            # redirect only；正式实现见 05_precision/czt
│   └── zoom_fft/                       # DRAFT docs/interface/TODO only
├── 05_precision/
│   ├── zero_cross_interpolation/       # implemented
│   ├── multi_cycle_average/            # implemented
│   ├── fft_parabolic_interpolation/    # implemented
│   ├── log_parabolic_interpolation/    # implemented
│   ├── multi_bin_energy/               # implemented
│   ├── window_gain_correction/         # implemented
│   ├── adc_gain_offset_calibration/    # implemented
│   ├── channel_delay_calibration/      # implemented
│   ├── robust_peak_to_peak/            # implemented
│   ├── robust_rms/                     # implemented
│   ├── sine_fit_3param/                 # implemented
│   ├── sine_fit_4param/                 # implemented, high-risk narrow search
│   ├── lock_in/                         # implemented
│   ├── jacobsen_interpolation/          # clean reimplementation / BUILD_VERIFIED
│   ├── quinn_interpolation/             # clean reimplementation / BUILD_VERIFIED
│   ├── macleod_interpolation/            # clean reimplementation / BUILD_VERIFIED
│   ├── coherent_sampling/               # clean reimplementation / BUILD_VERIFIED
│   ├── frequency_response_correction/   # clean reimplementation / BUILD_VERIFIED
│   └── czt/                             # unit-circle direct O(NM) / BUILD_VERIFIED
├── 10_tests/algorithm_pc/
│   ├── Makefile
│   ├── README.md
│   ├── test_helpers.h
│   ├── test_first_batch.c ... test_seventh_batch.c
│   ├── signal_test_vectors.c/.h
│   └── test_signal_vectors.c
└── tools/algorithm/
    └── README.md                        # 以后放系数/向量生成工具，当前不伪造
```

旧兼容目录只用于维护已有工程。当前推荐面为 14 个 Direct Recipe、2 个 Simple Helper，以及 Registry 中按 `FORMAL + selectable=true` 过滤后的正式算法；不要使用手工固定总数代替 Registry。

## 2 算法入口总表与状态

见 [ALGORITHM_LIBRARY_STATUS.md](ALGORITHM_LIBRARY_STATUS.md)：旧兼容 API 与正式模块的 234 项 PC 回归全部通过；9 个 SOURCE_LOST clean reimplementation 另有 Python reference、C/Python 对拍与 TI 完整链接证据。Zoom FFT 保持 DRAFT。当前无算法被标为 BOARD_VERIFIED 或 CONTEST_VERIFIED。

## 3 PC 验证清单

见 [algorithm_pc/README](../10_tests/algorithm_pc/README.md)。干净全量构建后 8 个测试程序共 234 PASS、0 FAIL。编译器为 PC GCC 严格 C11，不伪称 TI Arm Clang/板级验证。

## 4 模块接口矩阵

见 [MODULE_INTERFACE_MATRIX.md](MODULE_INTERFACE_MATRIX.md)，可查数据类型、单位、原地规则、可直接连接与错误连接。

## 5 算法选择、Pipeline 与 Recipe

- [ALGORITHM_SELECTION_GUIDE.md](ALGORITHM_SELECTION_GUIDE.md)：从测量目标和信号条件选方法。
- [SIGNAL_PROCESSING_PIPELINE_GUIDE.md](SIGNAL_PROCESSING_PIPELINE_GUIDE.md)：DC、Vpp、RMS、频率、频谱、THD、相位链。
- [ALGORITHM_RECIPES.md](ALGORITHM_RECIPES.md)：7 条 main.c 级拼装示例。
- [MEASUREMENT_RECIPE_INDEX.md](MEASUREMENT_RECIPE_INDEX.md)：频率、边沿、带宽、动态指标、自动控制与校准的完整测量逻辑链。
- [ALGORITHM_PITFALLS.md](ALGORITHM_PITFALLS.md)：12 个必须避免的误用。

## 6 DSP 与 FFT 资源预算

- [DSP_RESOURCE_BUDGET.md](DSP_RESOURCE_BUDGET.md)：复杂度、workspace、in-place 与 M0+ 软件浮点注意事项。
- [FFT_MEMORY_BUDGET.md](FFT_MEMORY_BUDGET.md)：512/1024/2048/4096 点逐数组 RAM；1024 Simple 优先，2048 仅经 map 验证的 RAM-saving，4096 不适合 32 KB RAM。

## 7 高级算法状态

Jacobsen、Quinn Second、Macleod、Coherent Sampling、Frequency Response Correction、CZT、DC Measure、FFT Peak 与 Duty 已按新规格完成 clean reimplementation，不是旧源码恢复，也不承诺旧 API drop-in；当前均为 `BUILD_VERIFIED`、Board `NOT_RUN`。Zoom FFT 和 [ALGORITHM_LIBRARY_STATUS.md](ALGORITHM_LIBRARY_STATUS.md) 明列的规划项仍为 DRAFT。

## 8 比赛优先推荐

1. 基础电压：ADC_ToVoltage + Calibration（若已标定）+ Mean/Vpp/RMS/AC_RMS。
2. 高 SNR 正弦频率：RemoveDC + rising ZeroCross + Interpolation + MultiCycleAverage。
3. 一般未知正弦频率/频谱：RemoveDC + Hann + 512/1024 FFT + Peak + Parabolic；幅值接 GainCorrection。
4. THD：Hann + Magnitude + true f0 + 多 bin Harmonic + THD，且禁止先乱低通。
5. 已知参考弱信号：LockIn，前提是参考与采样同步并已做延迟标定。
6. 偶发错误码：先确认尖峰不是目标，再使用 Hampel/RobustVPP/RMS，并记录处理计数。

## 9 不建议比赛现场临时启用的高风险算法

- SineFit4 未做赛题数据/初值范围扫测时；它是局部窄带搜索，不是全局搜索。
- 任意新 IIR/FIR 系数未离线验证幅频、相频和稳定性时。
- 2048 RAM-saving 未查看目标 map/栈水位时；4096 点本芯片 RAM 预算不成立。
- Correlation/Autocorrelation 使用巨大 lag 范围时，CPU 可能不可接受。
- Robust/Hampel/Median 用在真实脉冲、过冲、burst 或 THD 原始波形上。
- Zoom FFT 和状态表中仍为 DRAFT 的规划算法。

## 10 合入原则

算法已经位于唯一主库中，不存在第二个 Algorithms 仓库需要“合回”。比赛工程按模块 README 选择性冻结复制必要文件，记录来源，并重新完成 TI Arm Clang build、map RAM 检查和板级真值测试。
