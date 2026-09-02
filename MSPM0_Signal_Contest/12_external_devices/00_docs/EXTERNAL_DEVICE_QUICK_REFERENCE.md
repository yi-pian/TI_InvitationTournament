# External Device Quick Reference

## 我拿到器件后先看哪里

| 我看到的接口 | 打开 |
|---|---|
| SPI command/register | [SPI Register Recipe](recipes/SPI_REGISTER_DEVICE_RECIPE.md) |
| SPI ADC 串行数据帧 | [SPI Streaming ADC Recipe](recipes/SPI_STREAMING_ADC_RECIPE.md) |
| I2C address/register | [I2C Register Recipe](recipes/I2C_REGISTER_DEVICE_RECIPE.md) |
| 并行 ADC 总线 | [Parallel ADC Recipe](recipes/PARALLEL_ADC_RECIPE.md) |
| GPIO select/enable | [GPIO Controlled Recipe](recipes/GPIO_CONTROLLED_DEVICE_RECIPE.md) |
| CS/U-D/INC 三线 | [3-Wire GPIO Recipe](recipes/THREE_WIRE_GPIO_DEVICE_RECIPE.md) |
| VCTRL | [Analog Voltage Recipe](recipes/ANALOG_VOLTAGE_CONTROLLED_DEVICE_RECIPE.md) |
| PWM/duty | [PWM Recipe](recipes/PWM_CONTROLLED_DEVICE_RECIPE.md) |

## 本轮器件直接入口

- ADC：[ADS112C04](../adc/ads112c04/README.md) · [ADS7887](../adc/ads7887/README.md) · [ADS7866](../adc/ads7866/README.md)
- DAC：[DAC7811](../dac/dac7811/README.md)
- DDS：[AD9833](../dds/ad9833/README.md) · [AD9850](../dds/ad9850/README.md)
- PGA：[PGA113](../programmable_gain/pga113/README.md)
- 数字电位器：[TPL0401A-10](../digital_pot/tpl0401a_10/README.md) · [X9C104S](../digital_pot/x9c104/README.md)
- 模拟开关：[CD4052/4053](../analog_switch/cd4052_cd4053/README.md) · [CD4066B](../analog_switch/cd4066b/README.md) · [MAX14752](../analog_switch/max14752/README.md)
- GPIO 扩展：[TCA6408A](../gpio_expander/tca6408a/README.md)

## 现场固定顺序

```text
断电确认型号、供电和共地
→ 接线
→ 按 README 配 SysConfig
→ 先用 blocking / GPIO bit-bang
→ 只做一个固定读写动作
→ 仪器确认
→ 再提高速度或接入 Application
```

陌生 SPI SAR ADC：[只替换 mode/frame/bits/timing/command](SPI_SAR_ADC_MIGRATION_GUIDE.md)。

