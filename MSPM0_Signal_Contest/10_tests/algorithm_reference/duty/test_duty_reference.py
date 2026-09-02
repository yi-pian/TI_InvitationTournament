#!/usr/bin/env python3
"""Boundary and correctness checks for the independent Python reference."""

from __future__ import annotations

import math
import unittest

from duty_reference import (
    DutyConfig,
    DutyReferenceError,
    INSUFFICIENT_DATA,
    NO_FEATURE,
    NUMERIC_ERROR,
    OUT_OF_RANGE,
    measure_duty,
    synthesize_trapezoid,
)


class DutyReferenceTests(unittest.TestCase):
    def test_fractional_crossing_accuracy(self) -> None:
        samples = synthesize_trapezoid(400, 37.5, 0.3, phase_samples=0.37, transition_samples=5.0)
        result = measure_duty(
            samples,
            100_000.0,
            DutyConfig(level_mode="EXPLICIT", low_level=0.0, high_level=1.0, hysteresis_ratio=0.08),
        )
        self.assertLess(abs(float(result["duty_ratio"]) - 0.3), 2.0e-4)
        self.assertGreaterEqual(int(result["valid_cycle_count"]), 5)

    def test_incomplete_data(self) -> None:
        with self.assertRaisesRegex(DutyReferenceError, INSUFFICIENT_DATA):
            measure_duty([0.0, 1.0], 1.0)

    def test_no_cycle(self) -> None:
        with self.assertRaisesRegex(DutyReferenceError, NO_FEATURE):
            measure_duty([0.0, 0.0, 1.0, 1.0], 1.0)

    def test_constant_signal(self) -> None:
        with self.assertRaisesRegex(DutyReferenceError, NO_FEATURE):
            measure_duty([1.0] * 16, 1000.0)

    def test_nan_sample(self) -> None:
        with self.assertRaisesRegex(DutyReferenceError, NUMERIC_ERROR):
            measure_duty([0.0, math.nan, 1.0, 0.0], 1000.0)

    def test_invalid_hysteresis(self) -> None:
        with self.assertRaisesRegex(DutyReferenceError, OUT_OF_RANGE):
            measure_duty([0.0, 1.0, 0.0, 1.0, 0.0], 1000.0, DutyConfig(hysteresis_ratio=0.5))


if __name__ == "__main__":
    unittest.main()
