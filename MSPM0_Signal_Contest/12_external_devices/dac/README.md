# External DAC

README 类型：`CATEGORY_INDEX`

- [DAC7811](dac7811/README.md)：12-bit SPI multiplying/current-output DAC；必须外部参考和 I/V 运放。
- [TLC5615 family](tlc5615_family/README.md)：参考入口，非本轮正式驱动。
- [Generic SPI DAC](generic_spi_dac/README.md)：旧通用参考。

首次 SPI 接入看 [SPI Register Device Recipe](../00_docs/recipes/SPI_REGISTER_DEVICE_RECIPE.md)。MSPM0 内部 DAC/DAC DMA 仍使用 `01_bsp/`、`06_generator/` 的唯一源码。
