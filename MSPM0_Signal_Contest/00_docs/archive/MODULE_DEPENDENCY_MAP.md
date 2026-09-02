# Module dependency map

```text
01_bsp/common
  ├─ status / result / frame types
  ├─ 01_bsp hardware-neutral adapters
  │    ├─ system_clock -> timer
  │    ├─ adc / dma / timer -> acquisition
  │    └─ dac / opa / gpamp / comparator -> generator / frontend
  ├─ 02_acquisition
  │    ├─ adc_dma -> raw frame
  │    ├─ pingpong / ring / continuous -> stream frame
  │    └─ timer_capture / trigger_capture -> event metadata
  ├─ 03_measurement
  │    ├─ adc_to_voltage -> DC / Vpp
  │    ├─ mean/minmax -> DC / Vpp / oscilloscope
  │    ├─ zero-cross / interpolation / timer-capture -> frequency
  │    └─ duty / phase
  ├─ 04_dsp
  │    ├─ remove_dc / filters / windows
  │    ├─ FFT -> magnitude -> peak -> harmonic -> THD
  │    └─ correlation -> delay / phase calibration
  ├─ 05_precision
  │    ├─ zero-cross interpolation / multi-cycle average
  │    ├─ FFT parabolic / window gain / multi-bin energy
  │    ├─ coherent sampling / sine fit
  │    └─ ADC gain-offset / channel-delay calibration
  ├─ 06_generator
  │    ├─ wave table -> sine/square/triangle/saw/arbitrary
  │    ├─ table -> DDS -> DAC DMA
  │    └─ frequency sweep / AM
  ├─ 07_signal_frontend
  │    └─ OPA / GPAMP / comparator config -> BSP adapter
  └─ 08_applications
       └─ only combines the branches required by the selected recipe
```

## Ownership rule

- SysConfig owns pinmux, peripheral instances, clocks, Event Fabric, DMA channel and IRQ generation.
- BSP adapters own platform callbacks, not algorithms.
- Acquisition owns when and where samples arrive.
- Measurement/DSP/precision own array-to-result transforms and never access registers.
- Applications own sequencing and workspace reuse, not low-level reconfiguration.

## Dependency direction

Only depend downward or sideways on pure algorithms. A measurement module must not include an application; `adc_dma` must not depend on UART or FFT; a generator waveform must not depend on DAC hardware. This makes unused branches link-removable.
