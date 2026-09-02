# Duty TI Arm Clang target-link check

`validate.ps1` compiles the clean `signal_duty.c` implementation and a minimal caller for Cortex-M0+, then performs a complete TI link using the repository's already generated MSPM0G3507 device/startup/linker artifacts.

It proves target compile and link only. It does not prove SysConfig regeneration, board timing, ADC wiring or real-waveform accuracy.
