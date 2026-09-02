# Known System Limitations

## 已由构建证实

- 当前 Simple FFT 公共 API 需要 float input + float complex spectrum；Frequency C Q31 N=2048 完整链接因 `.bss=0x8010` 超过 32 KiB 失败。
- 4096 点不作为 PASS 目标，状态为 `UNSUPPORTED_BY_CURRENT_SIMPLE_FFT_API`。
- CMSIS Q31 相对 Reference 增加约 72.8 KiB Flash；四个核心 FFT 应用仍可放入 128 KiB Flash。
- N=1024 双通道 Phase 虽能链接，但 SRAM 仅余 3,040 B；默认 N=512。
- CMSIS Q15 在旧严格 THD/Phase 阈值各有一项小误差，不能作为稳定比赛默认。

## PENDING_BOARD

- IQMath/MATHACL 的实际 cycle、deadline 与能耗尚未做开发板 benchmark；Math Backend 保持 Reference。
- ADC 高 Fs 的采样保持、输入驱动能力、Timer 实际误差与 DMA 连续性需板上确认。
- DualADC 同步 skew、两路模拟前端群延迟和 ChannelDelayCalibration 效果需仪器确认。
- DAC settling、输出阻抗、幅值准确度、repeat block 边界杂散与 SFDR 需频谱仪确认。
- DDS 与 ADC 的 configured rate 来自整数 Timer 配置，不等于经校准的物理采样率。
- Sweep 当前以设定 DDS 幅度作为 reference；高精度 DUT 测量应增加参考通道或校准 DUT 端实际输入。
- Wave Replay AutoRange 会改变绝对 amplitude/offset，只保证归一化形状；周期选择依赖两次同向 trigger。
- 所有应用保留 512 B linker stack，但实际最大栈水位尚未测量。

## 模拟系统边界

- VREF、ADC gain/offset、前端增益/带宽/抗混叠、输入保护、源阻抗均会限制整机精度。
- MSPM0 内部 OPA/GPAMP 当前最终应用未启用；启用后必须重新进行 SysConfig 资源冲突和误差验证。
- PC Truth 只证明算法/Glue 对已知向量正确；Build 只证明工程闭环；二者都不能证明真实模拟性能。

比赛现场必须先用已知标准信号验证频率、幅值、相位和失真链，再将状态从 BUILD/PC 提升为 BOARD_VERIFIED。
