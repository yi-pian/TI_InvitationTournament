# Module Card: single_capture_replay

- Category: `02_acquisition`
- Recommendation: recommended for contest
- Status: `BOARD_VERIFIED`
- Main header: `signal_single_capture_replay.h`
- Purpose: continuous ADC + comparator-triggered single waveform capture, trimming, three-slot storage, safe ST7789 drawing and DAC DMA replay.
- Dependencies: `signal_dual_adc_mspm0g3507`, `signal_trigger_capture`, `signal_arbitrary_wave`, `signal_dac_dma_mspm0g3507`, `signal_tft_st7789`, `signal_status`.
- SysConfig: required by the dependency chain; read this module README and every selected hardware dependency README.
- Board evidence: `fuxian/example04`, MSPM0G3507, PA25 ADC0 + PA27 COMP0, 2026-08-19.
