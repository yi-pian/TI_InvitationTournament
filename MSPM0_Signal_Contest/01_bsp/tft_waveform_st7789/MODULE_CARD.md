# ST7789 Waveform Module

- **Layer:** BSP/display application helper
- **Status:** COPY_READY / COMPILE_VERIFIED
- **Hardware:** reuses `signal_tft_st7789` only; no additional SysConfig resource
- **Public header:** `signal_tft_waveform_st7789.h`
- **Public source:** `signal_tft_waveform_st7789.c`
- **Dependencies:** `signal_status.h`, `signal_tft_st7789.h`
- **Function:** decimated polyline or per-column min/max envelope with fixed/auto scale, grid, baseline, border and background controls
- **Frozen boundary:** application supplies samples and rectangle; module owns mapping and drawing loops
