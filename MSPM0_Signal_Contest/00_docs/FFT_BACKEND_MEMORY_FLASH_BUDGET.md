# FFT Backend RAM / Flash 预算

以下数据来自 TI Arm Clang 4.0.2.LTS 对 MSPM0G3507 的实际最终链接 `.map`。离线 smoke 使用 SDK 官方 startup/linker cmd 和预编译库；本轮 SysConfig CLI 因受限环境不能写 TI 用户缓存，因此不冒充“重新生成 SysConfig 已验证”。

## 1 公开 API 的真实 RAM

`SignalFFT_ForwardReal()` 当前接口同时接收 `float input[N]` 和 `signal_complex_f32_t spectrum[N]`：

| N | float 输入 | float 复频谱 | 两者合计 | smoke 含 512B stack | 目标链接 |
|---:|---:|---:|---:|---:|---|
| 512 | 2,048 B | 4,096 B | 6,144 B | 6,664 B | 成功 |
| 1024 | 4,096 B | 8,192 B | 12,288 B | 12,808 B | 成功 |
| 2048 | 8,192 B | 16,384 B | 24,576 B | 25,096 B | 成功，余量已小 |
| 4096 | 16,384 B | 32,768 B | 49,152 B | 需要约 49,668 B | 失败，`.bss=0xC004` |

所以仅把内部计算换成 Q15，不会改变旧公开 API 的 buffer 大小。4096 点在 32 KB SRAM 上不能使用这个 Simple 接口。

## 2 原生工作区比较

| Backend 原生复数工作区 | 每点字节 | 512 | 1024 | 2048 | 4096 |
|---|---:|---:|---:|---:|---:|
| CMSIS Q15，2N×int16 | 4 | 2,048 B | 4,096 B | 8,192 B | 16,384 B |
| CMSIS Q31，2N×int32 | 8 | 4,096 B | 8,192 B | 16,384 B | 32,768 B |
| CMSIS F32 / Reference complex | 8 | 4,096 B | 8,192 B | 16,384 B | 32,768 B |

真正省 RAM 的 Q15 路线要从 RAW 直接进入 Q15，并让后续 magnitude/energy 也留在定点；这需要新增专用定点链，不能假装旧 float API 已经省了一半。

## 3 实际 Flash

N=512 smoke；因为 SDK 预编译 CMSIS 库的表对象会带入多长度表，512/1024/2048 的 Flash 数值相同。

| Backend | Flash 使用 | 相对 Reference | SRAM 512 |
|---|---:|---:|---:|
| Reference C | 8,936 B | 基线 | 6,664 B |
| CMSIS Q15 | 55,712 B | +46,776 B | 6,664 B |
| CMSIS Q31 | 82,016 B | +73,080 B | 6,664 B |
| CMSIS F32 | 101,872 B | +92,936 B | 6,664 B |

CMSIS 档仍带 Reference fallback，以保持 2/4/8 点及大于 4096 的旧 API 行为。这也增加了一部分 Flash。静态库文件本身的几 MB 不是最终 Flash；必须看 `.map` 中实际拉入的 section。

## 4 标量数学 Flash smoke

同一 RMS+FFT-bin Phase 调用：

| Backend | Flash | SRAM（含 512B stack） | 状态 |
|---|---:|---:|---|
| Reference math.h | 3,192 B | 520 B | TARGET BUILD/LINK VERIFIED |
| IQMath RTS | 4,312 B | 516 B | IQMATH_TARGET_BUILD_VERIFIED |
| IQMath MATHACL library | 2,656 B | 516 B | MATHACL_TARGET_BUILD_VERIFIED |

这不是 cycle 结果。MATHACL 初始化和运行时共享仍需开发板验证。

## 5 选择建议

- 初学/兼容：Reference；
- 现有 Spectrum/THD/Phase 稳定迁移：CMSIS Q31，优先 1024；
- RAM 极紧且允许较小定点误差：设计原生 CMSIS Q15 链，不要只切 wrapper 宏；
- 2048：Simple API 能链接但只剩约 7.5 KB，集成应用加入其他全局变量后必须再看最终 `.map`；
- 4096：当前公开 API 禁用，不要现场硬上。
