# MODULE CARD

MODULE: Sine Fit 4-Parameter

CATEGORY: Precision / Sine Estimation

功能：在窄频率区间黄金分割搜索，每点做 3 参数正弦拟合。

输入：float V、initial/half-width/Fs Hz、iterations；输出 frequency、幅相/DC/residual。

是否原地处理：NO，输入只读。

依赖：SineFit3、math、公共状态码。

典型用途：粗频率后的单音精修。

不要用于：无初值、多音、宽范围搜索或硬实时 ISR。

计算量 HIGH O(I·N)，RAM O(1)。

Benefits：获得 bin 间频率和联合幅相。Trade-offs：局部单峰假设、CPU 高、现场参数敏感。

可连接：`CoarseFrequency -> SineFit4 -> Result`。

状态：PC_VERIFIED（干净窄带单音）；未实板，比赛前必须按题目复验。
