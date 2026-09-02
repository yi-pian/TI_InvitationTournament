# CZT 旧知识入口（不作为模块选择入口）

> 本目录仍然没有 `.c/.h`，不要从这里复制源码。CZT 已于 2026-08-13 在
> [`05_precision/czt`](../../05_precision/czt/README.md) 完成 clean reimplementation；
> 当前公开 API、限制和验证证据一律以该正式目录为准。

状态：`REDIRECT_ONLY`。本页只保留早期原理说明，不能代表当前实现状态。

CZT 可在用户指定的复平面弧段/频带上计算任意数量频点，适合窄带高密度观察，但它不会凭空增加采样记录包含的信息。快速 Bluestein 实现还需要卷积 FFT、chirp 系数、缩放和较大工作区；在 32 KB RAM 的 MSPM0G3507 上必须先做逐项预算。

Benefits：频率网格可定制。Trade-offs：参数复杂、内存和运算高、数值标度容易出错。

When NOT to use：宽带普通频谱、只需峰值插值、计算量未预算或没有 PC oracle 时。当前正式实现是小 M 窄带使用的 O(NM) 单位圆直接计算，不是 Bluestein 快速 backend。
