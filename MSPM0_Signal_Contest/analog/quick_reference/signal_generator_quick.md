---
id: analog.quick.signal_generator
title: 信号发生器 30 秒速查
kind: quick_reference
aliases: [信号发生器速查]
tags: [quick_reference, signal_generator]
summary: 现场快速确认Load、幅值单位、offset和实际负载端电压。
status: ENGINEERING_GUIDE
---

# 信号发生器 30 秒速查

1. 选波形/频率；确认幅值单位 Vpp 还是 Vrms。
2. 高阻DUT：Load=High-Z；50Ω链：Load=50Ω且末端单终端。
3. 检查 `Vmax=offset+Vpp/2`、`Vmin=offset-Vpp/2` 不超DUT。
4. 先接示波器在真实负载端测，再接DUT。
5. 面板1Vpp@50Ω接1MΩ常见约2Vpp。
6. SMA三通两个50Ω会成25Ω。
7. 台式发生器地多与保护地相连；浮地/高压连接前确认。

