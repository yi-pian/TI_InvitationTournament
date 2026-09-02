# 外部传感器：未来器件入口

README 类型：`FUTURE_CATEGORY_INDEX`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；当前没有确定料号或 Driver。

必须按真实量程、精度、响应时间、校准、供电、自热、接口与数据格式 Bring-Up。示例输出值不能当作测量仪器精度；模拟传感器还要核对前端、ADC 参考和保护。

- I2C 传感器：[I2C Register Device Recipe](../00_docs/recipes/I2C_REGISTER_DEVICE_RECIPE.md)
- SPI 传感器：[SPI Register Device Recipe](../00_docs/recipes/SPI_REGISTER_DEVICE_RECIPE.md)
- GPIO/脉冲传感器：[GPIO Controlled Device Recipe](../00_docs/recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md)
- 未知料号：[Unknown Device Bring-Up Guide](../00_docs/UNKNOWN_DEVICE_BRINGUP_GUIDE.md)

选定完整型号后建立独立目录，加入接线、数据换算、校准和最小 main。
