# Known Limitations

- Simple float-compatible FFT N=2048：完整 Frequency C 已因 SRAM 超限链接失败；每个 2048 应用都必须单独看 map，当前不作为默认。
- Simple FFT N=4096：`UNSUPPORTED_BY_CURRENT_SIMPLE_FFT_API`。需要正式端到端 Native Q15 pipeline，不能在集成层另写 FFT。
- FFT Backend 默认 CMSIS Q31。Q15 benchmark 可运行，但旧严格 THD/Phase 回归各有小误差，不作为稳定默认。
- Q31 相对 Reference 增加约 72.8 KiB Flash；更换 Backend 后必须重做 full link 和 PC truth。
- 普通 DSP 默认 SDK CMSIS-DSP；旧 Reference 只供 PC truth/旧 Application 兼容。IQMath/MATHACL 仅是标量热点的 `SPECIAL_BACKEND`，其 target cycle、deadline、能耗尚未实板 benchmark。
- ADC 高 Fs 的实际采样保持、输入驱动、Timer 误差和 DMA 连续性仍待开发板/仪器确认。
- DualADC 同步 skew、两路模拟前端群延迟和 delay calibration 效果仍待实板确认。
- DAC settling、幅值准确度、输出负载、DDS repeat block 杂散和 SFDR 仍待实板确认。
- Sweep 的 reference amplitude 当前来自 DDS 设定值，不是 DUT 输入端独立实测；绝对增益/相位需直通校准或参考通道。
- Wave Replay AutoRange 只保留归一化形状，会改变绝对 amplitude/offset；周期检测要求两个稳定同向 trigger。
- Linker `.stack=512 B` 不是运行时 stack high-water 证据；加入 UART/UI 后需要实测。
- BUILD/PC PASS 不等于 BOARD/CONTEST_VERIFIED；当前核心应用仍为 `PENDING_BOARD`。
