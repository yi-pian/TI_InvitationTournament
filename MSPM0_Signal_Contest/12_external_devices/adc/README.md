# External ADC

README 类型：`CATEGORY_INDEX`

- [ADS112C04](ads112c04/README.md)：I2C 16-bit 精密 ΔΣ ADC，适合低速小信号。
- [ADS7866](ads7866/README.md)：SPI 12-bit / 200-kSPS SAR ADC。
- [ADS7887](ads7887/README.md)：SPI 10-bit / 1.25-MSPS SAR ADC。
- [AD7606 family](ad7606_family/README.md)：并行/多通道 ADC 参考入口，非具体正式驱动。
- [Generic SPI ADC](generic_spi_adc/README.md)：旧通用参考。

陌生串行 SAR ADC 先看 [SPI Streaming ADC Recipe](../00_docs/recipes/SPI_STREAMING_ADC_RECIPE.md) 与 [迁移指南](../00_docs/SPI_SAR_ADC_MIGRATION_GUIDE.md)。MSPM0 内部 ADC 仍使用 `01_bsp/` 与 `02_acquisition/` 唯一源码。
