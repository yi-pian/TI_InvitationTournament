# 电源控制：未来器件入口

README 类型：`FUTURE_CATEGORY_INDEX`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；当前没有确定料号或 Driver。

这里包括 Load Switch、DC/DC 使能、电子负载和数字电源控制。先做限流空载、软启动、保护和温升检查，再接正式外部器件；GPIO 不能直接承担功率电流。

- EN/PG 控制：[GPIO Controlled Device Recipe](../00_docs/recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md)
- PWM 功率控制：[PWM Controlled Device Recipe](../00_docs/recipes/PWM_CONTROLLED_DEVICE_RECIPE.md)
- 数字电源总线：[I2C Register Device Recipe](../00_docs/recipes/I2C_REGISTER_DEVICE_RECIPE.md)
- 未知料号：[Unknown Device Bring-Up Guide](../00_docs/UNKNOWN_DEVICE_BRINGUP_GUIDE.md)

确定料号后单独记录上电顺序、默认电平、故障脚、限流值和安全关断路径。

