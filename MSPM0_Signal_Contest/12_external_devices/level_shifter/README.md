# 电平转换 / 总线缓冲：未来器件入口

README 类型：`FUTURE_CATEGORY_INDEX`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；当前没有确定料号或 Driver。

先区分推挽与开漏、单向与自动双向；再查两侧电源、VIH/VIL、DIR/OE、供电顺序、带宽和容性负载。自动双向器件不保证适合所有 SPI 或高速边沿。

- I2C 开漏总线：[I2C Register Device Recipe](../00_docs/recipes/I2C_REGISTER_DEVICE_RECIPE.md)
- SPI 推挽总线：[SPI Register Device Recipe](../00_docs/recipes/SPI_REGISTER_DEVICE_RECIPE.md)
- 纯 GPIO：[GPIO Controlled Device Recipe](../00_docs/recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md)
- 未知料号：[Unknown Device Bring-Up Guide](../00_docs/UNKNOWN_DEVICE_BRINGUP_GUIDE.md)

确定料号后单独记录方向、OE 上电状态和最大可靠速率。

