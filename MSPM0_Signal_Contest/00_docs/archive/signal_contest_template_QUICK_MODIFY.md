# Quick Modify

| 我要改变什么 | 文件/参数 | SysConfig | 说明 |
|---|---|---:|---|
| Processing Profile | `signal_features.h / SIGNAL_CONTEST_PROFILE` | 通常否 | Basic/Spectrum/THD/Phase |
| Fs/N | `signal_config.h / SAMPLE_RATE_HZ/SAMPLE_COUNT` | 否 | 改后必须 full link 看 map |
| ADC channel/pin | config 注释 + P06 profile | **是** | VREF 值本身不改路由 |
| VREF/scale/offset | `ADC_VREF_V/SIGNAL_INPUT_SCALE/SIGNAL_OFFSET_V` | 否 | 影响 code→V |
| FFT Backend | projectspec `SIGNAL_FFT_BACKEND` | 否 | 默认 Q31；main 不调 CMSIS |
| Math Backend | projectspec `SIGNAL_MATH_BACKEND` | MATHACL 资源需检查 | 默认 Reference |
| 频率范围/window | `EXPECTED_FREQ_* / WINDOW_TYPE` | 否 | peak、H5 Nyquist、泄漏 |
| trigger | `TRIGGER_LEVEL` | 否 | raw ADC code |
| DDS / sweep | `DDS_* / SWEEP_*` | 硬件路由不变时否 | 还需选择对应 Recipe Glue |
| DMA/Timer/Event/Comparator | P06 `.syscfg` | **是** | 重新 generate |

标准流程：复制模板 → 选 Recipe/Profile → 改参数 → 若改硬件路由则改 SysConfig → generate → compile/link → 检查 `.map` → PC truth → 实板仪器验证。
