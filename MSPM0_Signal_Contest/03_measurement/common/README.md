# 算法公共类型

这里仅放算法层共享的小型定义。目前只有 `signal_algorithm_status_t` 返回码，不放 ADC、DMA、Timer 或寄存器定义。

基础模块继续直接使用 `const float *samples`、`uint32_t count` 等接口，没有强制包装成通用 `signal_buffer_t`。原因是：比赛现场最常见的错误不是少一层封装，而是把数据类型、点数、单位或采样率传错。各模块会在参数名和 README 中明确这些信息。

编译时把本目录加入头文件搜索路径，例如：

```text
-I03_measurement/common
```
