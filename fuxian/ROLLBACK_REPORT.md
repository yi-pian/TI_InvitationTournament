# Feature refactor rollback report

## Outcome

The Feature/Analysis source layer has been removed. `example04/main.c` was restored byte-for-byte from the user-provided authoritative source `D:\Downloads\main(9).c`.

| Item | Result |
|---|---|
| Restored main SHA-256 | `808E12B07E8A76E5E56065A9452D3929F486A6FD44C5557B8511256D57E8231C` |
| `modules/features/` | removed |
| `examples_copyable/` | removed |
| `feature_tests/` | removed |
| Feature/Analysis source scan | 0 matches |
| SysConfig generation | PASS, warnings only |
| example04 source rebuild | BLOCKED: stale generated Debug makefile references intentionally absent legacy source files |
| Board validation | NOT_RUN |

## Removed, confirmed-new artifacts

- `example04/signal_contest_template_final/modules/features/`
- `examples_copyable/`
- `feature_tests/`
- `FEATURE_EXTRACTION_PLAN.md`
- `FEATURE_DEPENDENCY_GRAPH.md`
- `PHASE1_FFT_FREQUENCY_REPORT.md`
- `PHASE2_BASIC_MEASUREMENT_REPORT.md`
- `ANALYSIS_ENGINE_TEST_REPORT.md`
- `REFACTOR_CHANGELOG.md`
- orphan Feature object files and the stale Feature map/link-info files under `example04/.../Debug/`

## Restored files and provenance

| File | Classification | Action / source |
|---|---|---|
| `example04/signal_contest_template_final/main.c` | `MODIFIED_THIS_REFACTOR` | Replaced exactly from `D:\Downloads\main(9).c`; hash verified. It contains the original `App_RecipeADCToVoltage`, `App_RecipeCMSISSpectrumQ15`, `App_BasicMeasurements`, `App_TimeFrequency`, `App_Spectrum`, `App_RobustMeasurement` and `App_SineFitAndLockIn` chains. |
| `signal_config.h` | `UNCHANGED` | Not edited during rollback; no Feature reference. |
| `.cproject`, `.project`, `.ccsproject` | `UNCHANGED` | No Feature source/reference was found; no restoration edit was needed. |
| `README.md` | `UNCHANGED` | No restoration edit was made. |
| `COPIED_MODULES.md` | `UNKNOWN` | It was not modified during rollback. Its timestamp is within the work session, so no overwrite was attempted without an authoritative earlier copy. |
| existing `modules/*.c/.h` | `UNCHANGED` | No old atomic algorithm or hardware driver source was edited by this rollback. |

## CCS / SysConfig audit

`.cproject` contains no `modules/features`, `signal_analysis`, `signal_fft_frequency`, `signal_basic_measurement`, `signal_spectrum_core`, or related Feature entries. Clean was run, then the project SysConfig CLI regenerated `device_linker.cmd`, `device.opt`, `device.cmd.genlibs`, `ti_msp_dl_config.c/.h` successfully. SysConfig reported its existing ADC wake-up and peripheral-retention informational warnings only.

## Build status

The post-clean rebuild cannot proceed with the current generated `Debug/modules/subdir_vars.mk`: it lists legacy `.c` files which are absent from this project (`signal_ac_rms.c`, `signal_adc_to_voltage.c`, `signal_fft.c`, `signal_mean.c`, and related recipe compatibility sources). `COPIED_MODULES.md` explicitly states that those modules are intentionally not copied because the restored main uses the Recipe/CMSIS chains directly.

The generated Debug makefile is not edited manually, per the rollback requirement. A CCS project refresh/regeneration is required to recreate its source list from the current project; no missing source was invented or copied from another project. Therefore there is no current post-rollback map/RAM number. The former Feature map was removed rather than reported as a rollback result.

## Residual scan

The following Feature/Analysis patterns were scanned outside generated Debug outputs and this report:

```text
SignalFFTFrequency_
SignalBasicMeasurement_
SignalSpectrumAnalysis_
SignalAnalysis_
signal_analysis_context
signal_spectrum_core
signal_sample_format_t
modules/features
fft_execution_count
request_mask
```

Result: **0 matches**. No `features` directory or Feature object remains in example04.

## Exact-recovery limitations

The source restoration is exact for `main.c`, backed by the supplied file. `COPIED_MODULES.md` remains untouched because no verified pre-refactor copy was supplied. A complete compiler/link/map validation remains blocked only by the stale CCS-generated Debug source list; it requires CCS to regenerate that build metadata rather than a hand-maintained makefile.
