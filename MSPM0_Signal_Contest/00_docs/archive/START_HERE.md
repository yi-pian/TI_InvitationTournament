# START HERE

## 10 分钟建立比赛工程

1. 明确题目需要：输入通道数、最高频率、幅度范围、输出波形、显示/串口和误差要求。
2. 在 `CONTEST_MODULE_SELECTION.md` 选最短链路，不要默认加入 FFT、双通道或连续采集。
3. 在 `RECIPES.md` 复制最接近的组合，并先核算 RAM。
4. 新建 CCS/SysConfig 应用工程；正式模块仍从本仓库真实目录 linked file 引用，不复制源码。
5. include search path 指向实际物理目录；Project Explorer 的虚拟 `targetDirectory` 不是文件系统 include path。
6. 只在 `.syscfg` 选实例、引脚、Event、DMA 和 IRQ，应用代码使用生成宏。
7. 把 N、Fs、ADC 参考、量程、阈值、FFT 窗和 DDS 参数集中到应用 `signal_config.h`。
8. 先在 PC 跑 `10_tests/pc`；再用 TI Arm Clang 5.1.1.LTS Clean/Rebuild。
9. 上板时先验证最短硬件链，再逐模块加入算法；状态只按实际证据升级。
10. 比赛工程稳定后记录 `.map` RAM、实际 N/Fs、通道和验证输入。

## 最小数据流

```text
SysConfig hardware adapter
  -> uint16_t raw[N]
  -> adc_to_voltage (optional float[N])
  -> selected measurement/DSP
  -> application result
```

不用电压单位的算法可直接处理 raw；需要物理 RMS/Vpp/阈值时先换算或明确校准比例。

## 每次提交前

- 模块 API 是否能脱离 `main.c` 独立调用？
- N、Fs、通道是否只改配置/参数？
- 是否引入了新的隐藏全局数组？
- 32 KB SRAM 是否含栈、BSS、原始帧和全部工作区？
- `BUILD_VERIFIED` 是否被误写成实板通过？
- 不用该模块时能否只移除源文件与 SysConfig 资源？
