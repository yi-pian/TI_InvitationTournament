# Memory Map

当前资源与 Buffer 计算统一见 `00_docs/MEMORY_GUIDE.md`。当前 Simple FFT 典型数组：raw A/B=`4N` B，单通道 voltage=`4N` B，单 complex FFT=`8N` B，magnitude≈`2N` B；Phase 再增加 B voltage、B FFT 与 correlation。

- N=512：模板安全默认；双通道 Phase 推荐。
- N=1024：单通道 FFT 推荐；每次必须 full link 看 `.map`。
- N=2048：完整 Frequency C 已因 `.bss=0x8010` 超 32 KiB 失败，模板不承诺支持。
- N=4096：`UNSUPPORTED_BY_CURRENT_SIMPLE_FFT_API`。

链接器 512 B `.stack` 只是保留量，不是运行时高水位证据；Board 验证时必须测 stack 与 deadline。
