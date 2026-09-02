# External DDS

README 类型：`CATEGORY_INDEX`

- [AD9833](ad9833/README.md)：SPI、28-bit FTW、正弦/三角/方波。
- [AD9850](ad9850/README.md)：4-GPIO 40-bit serial、32-bit FTW；已有器件 core 与 MSPM0 platform。
- [Generic SPI DDS](generic_spi_dds/README.md)：旧参考入口。

DDS 只负责产生载波/周期信号。幅度、offset、输出滤波和驱动能力通常由外部模拟链完成；不要复制 `06_generator/dds` 的内部 DAC DDS 源码到这里。
