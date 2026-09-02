# 其他外部器件：分类入口

README 类型：`FUTURE_CATEGORY_INDEX`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`。

信号检测、整形、通信模块、数字逻辑、电机/执行器等尚未建立具体 Driver。先在 [External Device Catalog](../00_docs/EXTERNAL_DEVICE_CATALOG.md) 找类别，再按 [Unknown Device Bring-Up Guide](../00_docs/UNKNOWN_DEVICE_BRINGUP_GUIDE.md) 记录完整料号、供电、接口和风险。

根据真实接口选择 [SPI](../00_docs/recipes/SPI_REGISTER_DEVICE_RECIPE.md)、[I2C](../00_docs/recipes/I2C_REGISTER_DEVICE_RECIPE.md)、[GPIO](../00_docs/recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md)、[PWM](../00_docs/recipes/PWM_CONTROLLED_DEVICE_RECIPE.md) 或 [模拟电压控制](../00_docs/recipes/ANALOG_VOLTAGE_CONTROLLED_DEVICE_RECIPE.md) Recipe。没有确定器件前不创建伪 Driver。

