#!/usr/bin/env python3
from __future__ import annotations

import math
import unittest

from reference_models import (
    coherent_nearest, czt_unit_circle_real, dc_from_raw, fft_peak,
    frequency_response_correct, jacobsen, macleod, quinn_second,
    three_dft_bins,
)


class ReferenceTests(unittest.TestCase):
    def test_three_bin_estimators_scan(self):
        limits = {"jacobsen": 0.001, "quinn": 0.001, "macleod": 0.001}
        maximum = {name: 0.0 for name in limits}
        for size in (64, 128, 256):
            for peak in (5, 11, size // 4):
                for step in range(-45, 46):
                    delta = step / 100
                    bins = three_dft_bins(size, peak, delta)
                    estimates = {"jacobsen": jacobsen(bins),
                                 "quinn": quinn_second(bins),
                                 "macleod": macleod(bins)}
                    for name, estimate in estimates.items():
                        maximum[name] = max(maximum[name], abs(estimate - delta))
        for name, error in maximum.items():
            self.assertLess(error, limits[name], (name, error))

    def test_coherent_coprime(self):
        result = coherent_nearest(1000, 48000, 1024, 1, 511, True)
        self.assertEqual(math.gcd(result["cycles_per_record"], 1024), 1)

    def test_frequency_response_wrap(self):
        table = [{"frequency_hz": 100, "gain_correction_linear": 1.0,
                  "phase_correction_deg": 170},
                 {"frequency_hz": 1000, "gain_correction_linear": 1.0,
                  "phase_correction_deg": -170}]
        result = frequency_response_correct(table, 550, 2, 0, False, False)
        self.assertAlmostEqual(abs(result["applied_phase_correction_deg"]), 180)

    def test_czt_hits_tone(self):
        samples = [math.sin(2 * math.pi * 1000 * n / 8000) for n in range(64)]
        output = czt_unit_circle_real(samples, 8000, 800, 100, 5)
        self.assertEqual(max(range(5), key=lambda i: abs(output[i])), 2)

    def test_dc(self):
        result = dc_from_raw([0, 4095], 4095, 3.3, 1.0, 0.0)
        self.assertAlmostEqual(result["dc_voltage_v"], 1.65)

    def test_peak_first_tie(self):
        result = fft_peak([0, 2, 2, 1], 1, 3, 4000, 4)
        self.assertEqual(result["bin"], 1)


if __name__ == "__main__":
    unittest.main()
