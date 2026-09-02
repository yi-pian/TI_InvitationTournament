# MODULE CARD

MODULE: IIR Biquad / SOS

CATEGORY: DSP / Linear Filter

功能：用 Direct Form II Transposed 执行一个或多个二阶节级联。

输入：归一化 `a0=1` 的外部 SOS 系数、每节状态、输入块。

输出：滤波块，允许原地。

是否原地处理：YES。

依赖：公共算法状态码。

典型用途：用低阶实现较陡低通/高通/带通/陷波。

不要用于：未验证稳定性的系数、相位线性非常重要、THD 前滤掉谐波。

计算量：MEDIUM，O(N·S)，每节约 5 乘法。

RAM：每节状态 8 字节；系数 const 可放 Flash。

Benefits：少量节即可获得较陡响应；外部系数；状态跨块。

Trade-offs：通常非线性相位；系数/量化可导致不稳定；状态会产生启动瞬态。

可连接：`ADC_ToVoltage -> IIR Biquad -> measurement`。

状态：PC_VERIFIED。2026-08-07 通过可手算单节冲激响应；未完成稳定性/真实滤波器设计验证，未实板。
