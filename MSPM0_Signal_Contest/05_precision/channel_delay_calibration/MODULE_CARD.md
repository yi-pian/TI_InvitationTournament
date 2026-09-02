# MODULE CARD

MODULE: Channel Delay Calibration

CATEGORY: Precision / Phase Calibration

功能：由校准相位估计双通道固定时间差，并补偿当前频率的相位。

输入：B-A deg、Hz；输出 delay_s、corrected B-A deg。

是否原地处理：不适用。

依赖：公共状态码。

典型用途：DualADC 相位测量。

不要用于：未知整周歧义或频变群时延。

计算量 LOW O(1)，RAM O(1)。

Benefits：延迟随频率正确换算。Trade-offs：单一固定延迟模型。

可连接：`Phase -> DelayCalibration -> Result`。

状态：PC_VERIFIED；10/20 kHz 跨频率真值通过，未实板。
