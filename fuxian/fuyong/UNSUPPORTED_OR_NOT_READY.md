# 未生成或暂不就绪功能

| 项目/功能 | 状态 | 依据 |
|---|---|---|
| `03_adc_pingpong_dma` 硬件主题工程 | NOT_READY | `signal_adc_pingpong_dma` 是软件状态机；README 明确 DMA ISR 和下一目标寄存器由硬件适配层负责，仓库未找到可直接复用的同名 MSPM0 adapter + profile。不能猜 DMA 重装配置。 |
| Burst/edge/rise-fall | NOT_READY | 未在本轮审计中同时确认 README、真实 API、验证 example 和可复用 SysConfig。 |
| Lissajous/XY | NOT_READY | 未确认同一双 ADC/TFT 配置下的真实教学 API。 |
| 91 中的连续 DAC 表波 | 分流 | 不与 P07 DC 工程混用；已由 `90_dds_usage` + restored example04 DAC profile 覆盖。 |

以上项目未被伪造为“可用工程”。后续如补齐硬件 adapter 或已验证 profile，应从空母版独立建立主题工程。
