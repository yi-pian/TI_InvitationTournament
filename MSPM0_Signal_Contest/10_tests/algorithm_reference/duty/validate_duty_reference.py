#!/usr/bin/env python3
"""Scan clean/noisy synthetic cases and record duty-reference accuracy."""

from __future__ import annotations

import json
import math
from pathlib import Path

from duty_reference import DutyConfig, DutyReferenceError, measure_duty, synthesize_trapezoid


HERE = Path(__file__).resolve().parent


def summarize(errors: list[float]) -> dict[str, float | int]:
    return {
        "cases": len(errors),
        "bias": sum(errors) / len(errors),
        "rmse": math.sqrt(sum(error * error for error in errors) / len(errors)),
        "max_abs_error": max(abs(error) for error in errors),
    }


def main() -> int:
    grouped: dict[str, list[float]] = {"NOISELESS": [], "SNR_40_DB": [], "SNR_30_DB": []}
    failure_cases = []
    case_index = 0
    for period in (24.0, 37.5, 64.25, 127.75):
        for duty in (0.15, 0.30, 0.50, 0.70, 0.85):
            for phase in (0.11, 0.37, 0.73):
                for amplitude in (0.4, 1.0, 2.5):
                    for label, snr_db in (("NOISELESS", None), ("SNR_40_DB", 40.0), ("SNR_30_DB", 30.0)):
                        noise = 0.0 if snr_db is None else amplitude / (10.0 ** (snr_db / 20.0))
                        count = int(math.ceil(12.0 * period)) + 16
                        transition = min(8.0, 0.12 * period * min(duty, 1.0 - duty) / 0.15)
                        transition = max(3.0, transition)
                        samples = synthesize_trapezoid(
                            count=count,
                            period_samples=period,
                            duty_ratio=duty,
                            amplitude=amplitude,
                            offset=-0.2,
                            phase_samples=phase,
                            transition_samples=transition,
                            noise_rms=noise,
                            seed=3507 + case_index,
                        )
                        config = DutyConfig(
                            level_mode="EXPLICIT",
                            threshold_ratio=0.5,
                            hysteresis_ratio=0.08,
                            min_amplitude=1.0e-6,
                            low_level=-0.2,
                            high_level=-0.2 + amplitude,
                        )
                        try:
                            measured = measure_duty(samples, 100_000.0, config)
                            grouped[label].append(float(measured["duty_ratio"]) - duty)
                        except DutyReferenceError as error:
                            failure_cases.append({
                                "period_samples": period,
                                "duty_ratio": duty,
                                "phase_samples": phase,
                                "amplitude": amplitude,
                                "snr": label,
                                "status": error.status,
                            })
                        case_index += 1

    metrics = {label: summarize(errors) for label, errors in grouped.items()}
    output = {
        "reference": "duty_reference.py",
        "total_cases": case_index,
        "failure_count": len(failure_cases),
        "metrics": metrics,
        "failure_cases": failure_cases,
        "limits": {
            "NOISELESS_MAX_ABS": 2.0e-4,
            "SNR_40_DB_RMSE": 3.0e-3,
            "SNR_30_DB_RMSE": 1.0e-2,
        },
    }
    (HERE / "reference_metrics.json").write_text(
        json.dumps(output, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    print(json.dumps(output, indent=2, ensure_ascii=False))
    passed = (
        not failure_cases
        and metrics["NOISELESS"]["max_abs_error"] <= output["limits"]["NOISELESS_MAX_ABS"]
        and metrics["SNR_40_DB"]["rmse"] <= output["limits"]["SNR_40_DB_RMSE"]
        and metrics["SNR_30_DB"]["rmse"] <= output["limits"]["SNR_30_DB_RMSE"]
    )
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
