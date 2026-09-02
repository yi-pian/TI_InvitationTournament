# PC regression tests

These tests build every hardware-independent source with `-Wall -Wextra -Werror`, then exercise measurement, zero-cross frequency, FFT/peak interpolation, sine fitting, DDS, timer-capture math and ring-buffer behavior.

```powershell
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

`adc_dma` is intentionally excluded because it requires SysConfig-generated MSPM0 symbols and has its own CCS/board acceptance project.
