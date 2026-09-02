# SPI SAR ADC Recipe

这是比赛现场按名称搜索 `spi_sar_adc_recipe.md` 时的固定入口。

1. 第一次接线、SysConfig、阻塞读取 16-bit 帧：看 [SPI Streaming ADC Recipe](SPI_STREAMING_ADC_RECIPE.md)。
2. 从 ADS7887/ADS7866 改成陌生 XYZ ADC：看 [SPI SAR ADC Migration Guide](../SPI_SAR_ADC_MIGRATION_GUIDE.md)。
3. 只重新确认并修改：SPI mode、frame length、data bits/shift、CS/conversion timing、command；SPI Tx/Rx 和 GPIO CS 框架通常保留。

