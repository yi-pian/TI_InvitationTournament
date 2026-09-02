# 外部存储：未来器件入口

README 类型：`FUTURE_CATEGORY_INDEX`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；当前没有确定料号或 Driver。

先区分 EEPROM、NOR Flash、FRAM 等，再确认容量、页/扇区、写/擦周期、busy、耐久、掉电行为、地址宽度和写保护。最小实验是：读 ID/状态（若有）→ 写固定模式 → 等 busy → 读回 → 复位/掉电后再读。

- SPI 存储：[SPI Register Device Recipe](../00_docs/recipes/SPI_REGISTER_DEVICE_RECIPE.md)
- I2C EEPROM/FRAM：[I2C Register Device Recipe](../00_docs/recipes/I2C_REGISTER_DEVICE_RECIPE.md)
- 未知料号：[Unknown Device Bring-Up Guide](../00_docs/UNKNOWN_DEVICE_BRINGUP_GUIDE.md)

不要在未处理掉电和写寿命前把校准数据只放进单一外部存储副本。

