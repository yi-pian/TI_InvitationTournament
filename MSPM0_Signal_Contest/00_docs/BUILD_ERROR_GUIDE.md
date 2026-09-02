# Build Error Guide

只看日志中的**第一条根因**。按下表修正后，回到 Offline Workflow STEP 18，重新 Clean Build。

| 第一条错误/关键词 | 通常检查什么 | 修正后完成标志 |
|---|---|---|
| SysConfig error、PinMux、solution not found | `.syscfg` device/package/product、instance、pin、Timer/DMA/Event owner | SysConfig generate PASS；不手改生成文件 |
| `file not found`、include error | 主 `.h` 是否存在；compiler include path 是否为真实目录 | 该 header 可从当前 source include |
| compile error、incompatible type、too few arguments | 重新读正式 `.h` 的真实签名、结构、枚举、count/capacity | 所有 translation units compile PASS |
| generated macro/IRQ 未定义 | `.syscfg` instance 与生成 `ti_msp_dl_config.h` 的宏/ISR 名 | 应用只使用生成 header 中实际名字 |
| undefined symbol、unresolved symbol | API 对应唯一正式 `.c` 是否链接；Backend library/define 是否齐全 | final link 不再缺符号 |
| duplicate symbol、already defined | 同一 `.c` 是否被 link 两次；是否复制了模块源码 | 每个实现只出现一次，应用副本移除 |
| `.bss`、SRAM、placement、region overflow | N、FFT/dual Buffer、events、feature、stack；查看 linker 报告 | SRAM <32 KiB 且有可接受余量 |
| CMSIS/IQMath/MATHACL library error | Backend define、include、库和 CPU variant 是否一致 | 应用仍只调用公共 API，Backend 链接成功 |
| resource conflict、DMA/Timer/Event/IRQ | `RESOURCE_CONFLICT_GUIDE.md` owner 表和 `.syscfg` | 一个资源一个 owner，重新 generate PASS |

快速检查：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_projectspec_paths.ps1
```

成功标准只有一个：SysConfig PASS → 全部 `.c` Compile PASS → final Link PASS → 新 `.out` 与 `.map` 存在。单个 source check 不算成功。

