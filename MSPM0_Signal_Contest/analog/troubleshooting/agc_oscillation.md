---
id: analog.troubleshooting.agc_oscillation
title: AGC 来回振荡与锁不住故障树
kind: troubleshooting
aliases: [AGC振荡, VGA来回跳, 自动增益锁不住]
tags: [troubleshooting, agc, vga]
summary: 从量测时延、环路方向、标定、死区、步长和前级削顶定位AGC不稳定。
status: ENGINEERING_GUIDE
---

# AGC 来回振荡与锁不住故障树

1. **方向**：控制码增加实际增益是否单调增加；接反会正反馈。
2. **测量有效性**：削顶帧、切档过渡帧、未建立帧是否仍用于控制。
3. **时延**：更新后至少等待器件+滤波+一完整测量窗；否则控制旧数据。
4. **死区/步长**：死区太小、一步太大时在目标两侧跳。
5. **标定**：典型增益公式与实板曲线不符；用多频LUT。
6. **动态范围**：小信号到噪声底或大信号在VGA前已削顶，闭环无解。

先固定增益逐档验证模拟链，再开慢速闭环；记录每帧幅值、削顶标志、控制码与状态，不能只看最终屏幕值。

