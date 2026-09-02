# Resource Check

轻量 JSON Resource Manifest 冲突检查器。支持 `GPIO/PIN/ADC/DAC/SPI/UART/I2C/DMA/TIMER/EVENT/IRQ/OPA/GPAMP/COMP`；同一 `(type,id)` 出现多个 owner 时，除非所有声明都显式 `shareable=true` 且 `config` 完全一致，否则失败。

```powershell
python .\tools\resource_check\resource_check.py `
  .\tools\resource_check\profile_04_adc_dac.json
```

Manifest 只是 Agent 拼装前的明显冲突门禁，不能代替 `.syscfg`、生成 `ti_msp_dl_config.h`、原理图和 final link。共享 SPI 还必须逐器件核对 mode/bitrate/bit width/CS 与事务恢复；工具不会自动推断这些事实。

