# MODULE CARD

MODULE: Phase Methods

CATEGORY: Measurement / Phase

功能：统一 ZeroCross、FFT bin、Correlation lag 三种方法为 `phase_B-phase_A`。

输入：对应过零位置/复谱/lag 和周期；输出 deg/rad，范围 [-180,180)。

是否原地处理：不适用。

依赖：complex 类型、`atan2f/fmodf`。

典型用途：双 ADC 同步相位差。

不要用于：通道不同步却未做 delay calibration；两路频率不同却强行给单一相位。

计算量 LOW O(1)，RAM O(1)。

Benefits：三方法符号约定统一。Trade-offs：精度完全依赖上游过零/bin/lag与通道同步。

可连接：`DualADC -> RemoveDC -> method -> Phase`。

状态：PC_VERIFIED。解析 -18/+90/-90、完整 FFT +30、相关 -45 度通过。
