# MODULE CARD

MODULE: Lock-In / Synchronous Detection

CATEGORY: Precision / Synchronous Detection

功能：与已知正交参考积分，提取目标频率 I/Q、幅值和相位。

输入：float V、reference/Fs Hz、reference phase rad、remove_dc；输出 V、rad/deg。

是否原地处理：不修改输入。

依赖：math、公共状态码。

典型用途：已知 DDS 激励下的低 SNR 幅相/频响测量。

不要用于：未知频率、瞬态或参考不同步。

计算量 MEDIUM O(N)，RAM O(1)。

Benefits：窄带选择性和同步幅相。Trade-offs：响应速度换噪声带宽，依赖参考正确。

可连接：`Voltage -> LockIn -> Amplitude/Phase`。

状态：PC_VERIFIED；整数周期合成真值通过，未实板。
