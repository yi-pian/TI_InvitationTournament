# 可编程时钟 / PLL Synthesizer：未来器件入口

README 类型：`FUTURE_CATEGORY_INDEX`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；当前没有确定料号或 Driver。

选定型号后确认参考输入、VCO/分频范围、输出电平、相噪/抖动、锁定时间、Lock Detect、寄存器写序和上电校准。最小 Bring-Up 是固定频率 → 等 Lock → 用频率计/示波器验证，扫频放在最后。

- 寄存器型 SPI PLL：[SPI Register Device Recipe](../00_docs/recipes/SPI_REGISTER_DEVICE_RECIPE.md)
- GPIO/并行控制：[GPIO Controlled Device Recipe](../00_docs/recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md)
- 未知料号：[Unknown Device Bring-Up Guide](../00_docs/UNKNOWN_DEVICE_BRINGUP_GUIDE.md)

确定完整料号后必须使用官方寄存器图或厂商配置工具导出结果，不能凭同系列旧代码猜寄存器。

