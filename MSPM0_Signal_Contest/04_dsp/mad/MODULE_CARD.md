# MODULE CARD

MODULE: Median Absolute Deviation (MAD)

CATEGORY: DSP / Robust Statistics

功能：计算中位数、MAD 和 `1.4826*MAD` 鲁棒 sigma 估计。

输入：`samples[]`、`count`、`workspace[count]`。

输出：median、MAD、robust sigma，单位与输入相同。

是否原地处理：输入 NO；workspace 会被覆盖。

依赖：公共算法状态码。

典型用途：Hampel 离群检测、鲁棒噪声尺度估计。

不要用于：把 sigma 估计当频谱噪声密度；分布强烈非高斯仍机械使用 1.4826。

计算量：MEDIUM，Quickselect 平均 O(N)，最坏 O(N²)。

RAM：workspace `4*N` 字节，内部 O(1)。

Benefits：比均值/标准差更不容易被少量极端值拖动。

Trade-offs：需要整段 workspace；MAD=0 的离散/平坦数据需谨慎解释。

可连接：`samples -> MAD -> Hampel threshold/quality`。

状态：PC_VERIFIED。2026-08-07 通过已知中位数、MAD、缩放值测试；未实板验证。
