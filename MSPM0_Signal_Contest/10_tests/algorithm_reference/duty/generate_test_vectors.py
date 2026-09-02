#!/usr/bin/env python3
"""Generate deterministic typical duty-cycle vectors for PC/C tests."""

from __future__ import annotations

import json
from pathlib import Path

from duty_reference import DutyConfig, measure_duty, synthesize_trapezoid


HERE = Path(__file__).resolve().parent


def main() -> int:
    definitions = [
        dict(name="clean_25_percent", count=192, period=32.0, duty=0.25, phase=0.37, amplitude=1.2, offset=-0.1, transition=4.0, noise=0.0, seed=1),
        dict(name="fractional_period_50_percent", count=320, period=37.5, duty=0.50, phase=0.73, amplitude=2.0, offset=0.2, transition=5.0, noise=0.0, seed=2),
        dict(name="noisy_70_percent", count=512, period=48.0, duty=0.70, phase=0.11, amplitude=1.0, offset=-0.2, transition=5.0, noise=0.01, seed=3),
    ]
    vectors = []
    for definition in definitions:
        samples = synthesize_trapezoid(
            count=definition["count"],
            period_samples=definition["period"],
            duty_ratio=definition["duty"],
            amplitude=definition["amplitude"],
            offset=definition["offset"],
            phase_samples=definition["phase"],
            transition_samples=definition["transition"],
            noise_rms=definition["noise"],
            seed=definition["seed"],
        )
        config = DutyConfig(
            level_mode="EXPLICIT",
            threshold_ratio=0.5,
            hysteresis_ratio=0.08,
            min_amplitude=1.0e-6,
            low_level=definition["offset"],
            high_level=definition["offset"] + definition["amplitude"],
        )
        vectors.append({
            "name": definition["name"],
            "sample_rate_hz": 100000.0,
            "true_duty_ratio": definition["duty"],
            "config": config.__dict__,
            "samples": samples,
            "expected": measure_duty(samples, 100000.0, config),
        })
    (HERE / "test_vectors.json").write_text(
        json.dumps({"schema_version": 1, "vectors": vectors}, indent=2) + "\n",
        encoding="utf-8",
    )
    print(f"generated_vectors={len(vectors)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
