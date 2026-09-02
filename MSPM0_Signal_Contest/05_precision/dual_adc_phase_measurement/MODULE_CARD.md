# MODULE CARD

MODULE: Dual ADC Phase Measurement

CATEGORY: Precision / Timing

功能：从同一帧双路同步 ADC 原始码中检测 X/Y 上升过零点，插值得到小数采样位置，并计算 Y 相对 X 的相位差。

输入：两路等长、同一采样率、已完成 DMA 的 `uint16_t` 原始码数组；已知频率比 `fY/fX=1~5`。

输出：`phase_degrees`，范围 `[-180,180]`，正值表示 Y 超前；同时输出有效过零点数量用于诊断。

是否原地处理：NO。

依赖：公共算法状态码；上游双 ADC 同步采集模块提供数据和真实采样率。

典型用途：X=1.5~2 kHz、Y 为 X 的 1~5 倍时的相位差测量。

不要用于：两路不同步、Y/X 频率比未知、采样未完成或输入幅度不足的情况。

计算量：MEDIUM，O(N+K)，N 为样本数，K 为过零点数。

RAM：模块内部固定数组约 `(32+128)*4 + 32*2 = 704` 字节，调用者数组不计入。

Benefits：不需要浮点、FFT 或动态内存；同时支持动态中点、滞回和亚采样插值。

Trade-offs：依赖输入幅度、过零附近采样质量和正确的频率比；窗口过短时可能没有足够过零点。

可连接：`Dual ADC Sync -> Dual ADC Phase Measurement -> TFT/UART`。

SysConfig：不需要；ADC、Timer、DMA、Event 和引脚仍由上游采集模块配置。

状态：PC_VERIFIED；已完成 TI Arm Clang 主机端源码级检查，22_X 工程和实板验证仍需分别确认。
