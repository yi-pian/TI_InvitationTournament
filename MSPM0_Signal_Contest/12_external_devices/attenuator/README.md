# 可编程衰减器：未来器件入口

README 类型：`FUTURE_CATEGORY_INDEX`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；当前没有确定料号或 Driver。

拿到器件后先确认工作频段、衰减范围/步进/误差、插损、最大输入功率、P1dB/IP3、回波损耗、控制接口和上电档位。最小验证只在安全低功率下测 0 dB、中间、最大三个档位。

- SPI/寄存器控制：[SPI Register Device Recipe](../00_docs/recipes/SPI_REGISTER_DEVICE_RECIPE.md)
- 并行/锁存 GPIO：[GPIO Controlled Device Recipe](../00_docs/recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md)
- 未知料号：[Unknown Device Bring-Up Guide](../00_docs/UNKNOWN_DEVICE_BRINGUP_GUIDE.md)

选定完整料号后新建独立器件目录，再写 Pin、真值表、时序和可运行示例。

