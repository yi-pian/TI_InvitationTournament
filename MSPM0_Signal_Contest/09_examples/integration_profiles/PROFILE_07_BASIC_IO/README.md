# PROFILE 07：Basic I/O Closure

用于平台闭环最小工程：软件触发 ADC0 单点采样、DAC0 固定电压、UART0 基础收发和 GPIO 输出。

| 功能 | 外设 | 引脚 | 关键配置 |
|---|---|---|---|
| ADC Basic | ADC0 MEM0 / channel 2 | PA25 | software trigger, 12-bit, VDDA 3.3 V |
| DAC DC | DAC0 OUT | PA15 | 12-bit, VDDA/VSSA, amplifier on |
| UART | UART0 | PA10 TX / PA11 RX | 115200 8-N-1, FIFO, internal loopback off |
| GPIO | `SIGNAL_GPIO/OUTPUT` | PA12 | output, initial low |

本 Profile 只是可编译参考；未做开发板验证。ADC 与 DAC 电压范围按板上 VDDA 实测值为准。
