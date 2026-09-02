# 外设 API 破坏性变更记录

本文件只记录需要调用方迁移的外设 public API/语义变更。普通文档修正、SysConfig profile 增加和不改变接口的 bug fix 不列为 breaking change。

## 2026-08-07：采样率 getter 语义更名

旧名称：

```c
SignalADC_GetActualSampleRate()
```

新名称：

```c
SignalADC_GetConfiguredTriggerRate()
```

原因：返回值由 Timer source clock 和整数 `timer_count` 计算，并非外部仪器或独立时钟参考实测采样率。“Actual” 会误导算法和验收人员。

迁移：替换函数名，并在数据契约中把该值标记为 configured。需要物理采样率时必须由独立参考测量并另行传入，不能重用本 getter 的语义。

## 2026-08-07：公共 API 冻结基线

40 个外设模块及 `01_bsp/common` 的公共头文件自此建立 hash manifest。冻结本身不改变调用代码；后续若改变已有签名、结构体布局、枚举值或既有语义，必须在本文件追加：

1. 旧接口；
2. 新接口；
3. 变更原因；
4. 调用方迁移步骤；
5. 受影响 Demo/profile/template；
6. 新旧版本是否可并存。

工程 Include Search Path 从虚拟 `modules/*` 修正到真实源目录不属于 API breaking change；它修复的是 CCS 工程集成错误，正式模块 source of truth 未改变。
