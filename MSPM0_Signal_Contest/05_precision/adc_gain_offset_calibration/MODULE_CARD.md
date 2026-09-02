# MODULE CARD

MODULE: ADC Gain/Offset Calibration

CATEGORY: Precision / Calibration

功能：用两点直线模型修正 ADC 电压比例与零点误差。

输入：measured/true 两点 V；输出 gain（无量纲）、offset_v 和校准电压 V。

是否原地处理：YES（Apply）。

依赖：公共状态码。

典型用途：ADC_ToVoltage 后的系统刻度校准。

不要用于：非线性、温漂、削顶或频响误差。

计算量：LOW，Apply O(N)；RAM O(1)。

Benefits：接口简单、无动态内存。Trade-offs：只拟合一条直线，准确度不可能超过参考源。

可连接：`ADC_ToVoltage -> Calibration -> Measurement/DSP`。

状态：PC_VERIFIED；两点真值与应用测试通过，未实板。
