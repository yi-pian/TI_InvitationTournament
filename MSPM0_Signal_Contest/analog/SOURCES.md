---
id: analog.navigation.sources
title: 权威来源与引用规则
kind: navigation
aliases: [模拟资料来源, 官方datasheet来源]
tags: [sources, datasheet, contest]
summary: 历年题目和器件官方资料入口，以及禁止把典型值或芯片数据冒充模块实测的规则。
status: OFFICIAL_SOURCE_VERIFIED
---

# 权威来源与使用规则

## 历年题目

- 全国大学生电子设计竞赛培训网历年赛题入口：<https://www.nuedc-training.com.cn/index/download/download_list/type/1>
- 2018 A 电流信号检测装置：<https://www.nuedc-training.com.cn/index/news/details/new_id/53>
- 2019 D 简易电路特性测试仪：<https://www.nuedc-training.com.cn/index/news/details/new_id/149>
- 2019 E 基于互联网的信号传输系统：<https://www.nuedc-training.com.cn/index/news/details/new_id/150>
- 2021 赛题总页：<https://res.nuedc-training.com.cn/topic/2021/topic_from_3.html>
- 2023 D 信号调制方式识别与参数估计：<https://res.nuedc-training.com.cn/topic/2023/topic_98.html>
- 2023 H 信号分离装置：<https://res.nuedc-training.com.cn/topic/2023/topic_102.html>
- 2024 C 无线传输信号模拟系统：<https://res.nuedc-training.com.cn/topic/2024/topic_111.html>
- 2025 F 简易自动接收机：<https://res.nuedc-training.com.cn/topic/2025/topic_125.html>
- 2025 G 电路模型探究装置：<https://res.nuedc-training.com.cn/topic/2025/topic_126.html>

题面为图片且无法可靠提取数字时，本库只写题目已确认的任务特征，不补猜幅度、频率或精度。

## 器件官方资料

- TI MSPM0G3507：<https://www.ti.com/product/MSPM0G3507>
- AD9226 Rev.B：<https://www.analog.com/media/en/technical-documentation/data-sheets/ad9226.pdf>
- AD603 Rev.K：<https://www.analog.com/media/en/technical-documentation/data-sheets/ad603.pdf>
- AD9850 Rev.H：<https://www.analog.com/media/en/technical-documentation/data-sheets/ad9850.pdf>
- AD9833 Rev.G：<https://www.analog.com/media/en/technical-documentation/data-sheets/AD9833.pdf>
- STM32F407/417：<https://www.st.com/en/microcontrollers-microprocessors/stm32f407-417.html>
- STM32H743/753：<https://www.st.com/en/microcontrollers-microprocessors/stm32h743-753.html>

## 引用规则

1. 芯片具体供电、输入范围、GBW、SR、采样率、ENOB、失真、逻辑电平与引脚必须来自 exact datasheet。
2. 模块板可能改变时钟、滤波、增益、输出幅值和接口电平；芯片数据不能直接冒充模块实测。
3. `typical` 不能当保证值；极限设计看 min/max 与测试条件。
4. Datasheet 验证不等于 `BOARD_VERIFIED`。
