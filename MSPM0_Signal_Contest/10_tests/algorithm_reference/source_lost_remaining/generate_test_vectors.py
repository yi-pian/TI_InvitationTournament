#!/usr/bin/env python3
from __future__ import annotations

import json
import math
from pathlib import Path

from reference_models import (
    coherent_nearest, czt_unit_circle_real, dc_from_raw, fft_peak,
    frequency_response_correct, jacobsen, macleod, quinn_second,
    three_dft_bins,
)


HERE = Path(__file__).resolve().parent
OUT = HERE / "test_vectors"


def dump(name: str, value: object) -> None:
    (OUT / f"{name}.json").write_text(
        json.dumps(value, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def complex_pairs(values: list[complex]) -> list[list[float]]:
    return [[value.real, value.imag] for value in values]


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    estimator_vectors = []
    for size, peak, delta in [(64, 9, -0.37), (128, 17, 0.23), (256, 41, 0.44)]:
        bins = three_dft_bins(size, peak, delta)
        estimator_vectors.append({
            "fft_size": size, "peak_index": peak, "true_delta": delta,
            "sample_rate_hz": 48000.0, "bins": complex_pairs(bins),
            "jacobsen": jacobsen(bins), "quinn_second": quinn_second(bins),
            "macleod": macleod(bins),
        })
    for name in ("jacobsen_interpolation", "quinn_interpolation",
                 "macleod_interpolation"):
        dump(name, {"vectors": estimator_vectors})

    coherent_cases = [
        (997.0, 48000.0, 1024, 1, 511, False),
        (1000.0, 48000.0, 1024, 1, 511, True),
        (1234.5, 20000.0, 1000, 3, 400, True),
    ]
    dump("coherent_sampling", {"vectors": [
        {"input": {"desired_frequency_hz": c[0], "sample_rate_hz": c[1],
                   "sample_count": c[2], "minimum_cycles": c[3],
                   "maximum_cycles": c[4], "require_coprime": c[5]},
         "expected": coherent_nearest(*c)} for c in coherent_cases]})

    table = [
        {"frequency_hz": 100.0, "gain_correction_linear": 1.1,
         "phase_correction_deg": 170.0},
        {"frequency_hz": 1000.0, "gain_correction_linear": 1.0,
         "phase_correction_deg": -170.0},
        {"frequency_hz": 10000.0, "gain_correction_linear": 0.9,
         "phase_correction_deg": -130.0},
    ]
    frc_vectors = []
    for frequency, log_mode, clamp in [(550.0, False, False),
                                       (math.sqrt(1000.0 * 10000.0), True, False),
                                       (50.0, False, True)]:
        frc_vectors.append({"table": table, "frequency_hz": frequency,
                            "measured_gain_linear": 2.0,
                            "measured_phase_deg": 25.0,
                            "log_frequency": log_mode, "clamp": clamp,
                            "expected": frequency_response_correct(
                                table, frequency, 2.0, 25.0, log_mode, clamp)})
    dump("frequency_response_correction", {"vectors": frc_vectors})

    samples = [0.3 + math.sin(2 * math.pi * 1100 * n / 8000)
               + 0.2 * math.cos(2 * math.pi * 1700 * n / 8000)
               for n in range(32)]
    czt_expected = czt_unit_circle_real(samples, 8000.0, 900.0, 100.0, 10)
    dump("czt", {"vectors": [{"samples": samples, "sample_rate_hz": 8000.0,
                                "start_frequency_hz": 900.0,
                                "frequency_step_hz": 100.0,
                                "expected": complex_pairs(czt_expected)}]})

    raw = [1000, 1100, 1200, 1300, 1400]
    dump("dc_measure", {"vectors": [{"raw": raw, "adc_max_code": 4095,
                                       "reference_voltage_v": 3.3,
                                       "input_scale": 2.0,
                                       "offset_voltage_v": -1.0,
                                       "expected": dc_from_raw(raw, 4095, 3.3, 2.0, -1.0)}]})
    values = [0.1, 0.4, 2.0, 1.2, 2.0, 0.3]
    dump("fft_peak", {"vectors": [{"values": values, "first_bin": 1,
                                     "last_bin": 5, "sample_rate_hz": 12000.0,
                                     "fft_size": 12,
                                     "expected": fft_peak(values, 1, 5, 12000, 12)}]})


if __name__ == "__main__":
    main()
