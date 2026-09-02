# MODULE CARD

MODULE: SFDR

CATEGORY: DSP / Quality Metric

功能：主 band 最大谱线 / band 外最大杂散谱线，输出 ratio/dB。

输入：magnitude、main/analysis ranges；输出主峰/杂散 bin/value/SFDR。

是否原地处理：不适用。

依赖：`log10f`。

典型用途：查最大 spur 与无杂散动态范围。

不要用于：主瓣未完整排除、DC/泄漏被误认 spur、把 SFDR 当 SNR。

计算量 O(B)，RAM O(1)。

Benefits：同时报告 spur 位置。Trade-offs：对窗泄漏和 main band 宽度敏感。

状态：PC_VERIFIED；主10/spur2真值13.9794006 dB通过。
