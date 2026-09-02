# MODULE CARD

MODULE: Multi-bin Energy

CATEGORY: Precision / Spectrum Energy

功能：对中心 bin 左右半径内 magnitude² 求和。

输入：magnitude、center_bin、radius；输出实际范围、energy、RSS。

是否原地处理：不适用。

依赖：公共状态码。

典型用途：非整 bin/加窗后的基波与谐波能量积分。

不要用于：相邻主瓣窗口重叠、把 raw energy 直接叫 V² 而未说明标度。

计算量 LOW O(2R+1)，RAM O(1)。

Benefits：比单 bin 更不易漏掉泄漏到邻 bin 的能量。Trade-offs：半径大把噪声/邻近音也算入。

可连接：`Magnitude -> MultiBinEnergy -> Harmonic/THD`。

状态：PC_VERIFIED。3-4-5 三角真值能量 25、RSS 5 通过。

24_C 用法：输入为线性 magnitude，Hann 频谱下以 radius_bins=2 为起点积分 H1~H5 邻近 bin；窗口不可重叠，H5 越 Nyquist 时跳过并检查返回码。
