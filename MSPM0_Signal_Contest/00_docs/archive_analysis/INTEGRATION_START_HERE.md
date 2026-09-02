# Integration Start Here

如果你是初学者，先不要打开底层 `.c`。按下面的“我要做什么”进入已经完整链接的应用。

| 我要做什么 | 先打开 | Recipe | 默认起点 |
|---|---|---|---|
| 测 DC/Vpp/RMS | `08_applications/signal_meter/README.md` | 01~03 | P01，100 kS/s，N=1024 |
| 测频率 | `08_applications/frequency_meter/README.md` | 04~06 | 边沿 A；正弦 B；噪声/失真 C |
| 做 FFT/看频谱 | `08_applications/spectrum_analyzer/README.md` | 07 | Q31，N=1024 |
| 测 H2~H5/THD | `08_applications/harmonic_thd_analyzer/README.md` | 08 | Q31，N=1024 |
| 测双通道相位 | `08_applications/dual_channel_phase_meter/README.md` | 09 | Q31，N=512 |
| 输出正弦 | `08_applications/dds_generator/README.md` | 10 | P03 |
| 做扫频 | `08_applications/sweep_analyzer/README.md` | 11 | P04 + 外部 DUT |
| 捕获并重放 | `08_applications/waveform_capture_replay/README.md` | 12/13 | P04 |
| 组合多种测量 | `08_applications/signal_analyzer/README.md` | 按 Profile | 只启用需要的功能 |
| 复现赛题 | `contest_reproductions/README.md` | 先做题目分析 | 复制 Contest Template |

## 最短工作流

1. 在 `SYSTEM_RECIPE_SELECTION.md` 选 Recipe。
2. 打开对应完整应用；若题目是组合功能，复制 `08_applications/signal_contest_template/`。
3. 在 `signal_config.h` 改 Fs、N、VREF、频率范围等；在 `signal_features.h` 选功能。
4. 只有更换 pin、ADC 通道、DMA、Timer/Event、Comparator 时才改 SysConfig。
5. 导入 `ticlang/*.projectspec`，运行 SysConfig generate，然后 TI Arm Clang Build。
6. 必须看到最终 `.out` 与 `.map`；只做 source check 不算完成。
7. 对照 `FINAL_INTEGRATION_BUILD_MATRIX.md` 和 `.map` 检查 Flash/SRAM。
8. PC 测试通过后接开发板和仪器；在此之前状态是 `PENDING_BOARD`。

比赛现场推荐：FFT 用 CMSIS Q31；标量 Math 保持 Reference；单通道 FFT N=1024，双通道 Phase N=512。不要把 N=2048/4096 直接写进 Simple FFT 模板。
