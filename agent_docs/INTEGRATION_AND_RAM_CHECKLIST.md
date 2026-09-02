# 最终集成与 RAM 自审

在输出“可以进入实现阶段”前完成本表，信息必须来自 README、`.h/.c`、`.syscfg`、生成 header 或官方资料。

## 数据契约

- 上一模块 output type 与下一模块 input type。
- `uint16_t raw / float V / Q15/Q31 / complex`。
- count、capacity、字节数、FFT N、单边 `N/2+1`。
- buffer owner、写入期、只读期、生命周期。
- in-place 是否由当前 `.h/.c` 明确支持。
- Fs 单位/真实来源、V/Hz/rad/deg/ratio/%/RMS/peak。
- VREF、ADC max code、前端 gain/offset、FFT/window scaling。
- Init/Start/Done/valid 和返回码。

## 硬件契约

DMA、Timer、SPI/I2C/UART、IRQ、Event、SysConfig instance 和 Pin 必须有唯一 owner；共享总线的 mode/bitrate/CS/事务恢复必须一致。关于 buffer 复用、DMA、SPI、IRQ、SysConfig、Pin 不允许凭经验猜。

## RAM

不再用“所有数组相加 < SRAM”作为唯一结论：

```text
buffer 生命周期
-> 最大同时存活 buffer
+ FFT/backend scratch
+ globals
+ stack reservation/high-water
+ library workspace
```

已有 build 优先读 `.map`；`tools/ram_check/ram_check.py` 可解析 TI map SRAM 行并检查生命周期 Manifest。Map 的链接 stack reservation 不是运行时 stack high-water；in-place/buffer 复用必须有 API 证据，不能为了降 RAM 擅自覆盖仍在使用的数据。

## 实现准入结果

输出简短 owner 表、数据链、peak live RAM、依据路径、尚不确定项。任何关键 instance/Pin/ownership/in-place/单位无法确认时，状态是“信息不足，需要补证据”，不是“可以实现”。

