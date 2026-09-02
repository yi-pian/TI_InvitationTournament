# Analog Switch / MUX

README 类型：`CATEGORY_INDEX`

- [CD4052B/CD4053B](cd4052_cd4053/README.md)：双路 4:1 / 三组 2:1，GPIO 地址控制。
- [CD4066B](cd4066b/README.md)：四路独立双向 SPST，GPIO 控制。
- [MAX14752](max14752/README.md)：8:1 高压 MUX；必须高压安全设计。
- [CD4051/74HC4051](cd4051_74hc4051/README.md)：既有 8:1 参考入口。

简单开关不建 `.c/.h`。使用 [GPIO Controlled Device Recipe](../00_docs/recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md)，同时核对模拟电压范围、RON、带宽、漏电、逻辑 VIH 和上电默认状态。
