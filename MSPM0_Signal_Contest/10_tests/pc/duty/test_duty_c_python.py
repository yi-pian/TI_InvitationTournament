#!/usr/bin/env python3
"""Build the C implementation and compare it with the Python reference."""

from __future__ import annotations

import argparse
import ctypes
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import tempfile


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
REFERENCE = ROOT / "10_tests/algorithm_reference/duty"
sys.path.insert(0, str(REFERENCE))
from duty_reference import DutyConfig, measure_duty  # noqa: E402


class CConfig(ctypes.Structure):
    _fields_ = [
        ("level_mode", ctypes.c_int),
        ("threshold_ratio", ctypes.c_float),
        ("hysteresis_ratio", ctypes.c_float),
        ("min_amplitude", ctypes.c_float),
        ("low_level", ctypes.c_float),
        ("high_level", ctypes.c_float),
    ]


class CResult(ctypes.Structure):
    _fields_ = [
        ("duty_ratio", ctypes.c_float),
        ("duty_percent", ctypes.c_float),
        ("period_s", ctypes.c_float),
        ("frequency_hz", ctypes.c_float),
        ("high_width_s", ctypes.c_float),
        ("low_width_s", ctypes.c_float),
        ("low_level", ctypes.c_float),
        ("high_level", ctypes.c_float),
        ("threshold_level", ctypes.c_float),
        ("valid_cycle_count", ctypes.c_uint32),
        ("rising_edge_count", ctypes.c_uint32),
        ("falling_edge_count", ctypes.c_uint32),
    ]


FLOAT_FIELDS = (
    "duty_ratio", "duty_percent", "period_s", "frequency_hz",
    "high_width_s", "low_width_s", "low_level", "high_level",
    "threshold_level",
)
COUNT_FIELDS = ("valid_cycle_count", "rising_edge_count", "falling_edge_count")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    source = ROOT / "03_measurement/duty/signal_duty.c"
    duty_include = ROOT / "03_measurement/duty"
    common_include = ROOT / "03_measurement/common"
    vectors = json.loads((REFERENCE / "test_vectors.json").read_text(encoding="utf-8"))["vectors"]

    with tempfile.TemporaryDirectory() as directory:
        library_path = Path(directory) / "signal_duty.dll"
        command = [
            args.cc, "-shared", "-O2", "-std=c11", "-Wall", "-Wextra", "-Werror",
            f"-I{duty_include}", f"-I{common_include}", str(source), "-o", str(library_path),
        ]
        subprocess.run(command, check=True)
        library = ctypes.CDLL(str(library_path))
        process = library.SignalDuty_Process
        process.argtypes = [
            ctypes.POINTER(ctypes.c_float), ctypes.c_uint32, ctypes.c_float,
            ctypes.POINTER(CConfig), ctypes.POINTER(CResult),
        ]
        process.restype = ctypes.c_int

        max_abs_error = 0.0
        max_abs_error_by_field = {field: 0.0 for field in FLOAT_FIELDS}
        comparisons = 0
        for vector in vectors:
            raw_config = vector["config"]
            py_config = DutyConfig(**raw_config)
            expected = measure_duty(vector["samples"], vector["sample_rate_hz"], py_config)
            c_config = CConfig(
                0 if raw_config["level_mode"] == "AUTO_MIN_MAX" else 1,
                raw_config["threshold_ratio"], raw_config["hysteresis_ratio"],
                raw_config["min_amplitude"], raw_config["low_level"], raw_config["high_level"],
            )
            sample_array = (ctypes.c_float * len(vector["samples"]))(*vector["samples"])
            actual = CResult()
            status = process(sample_array, len(vector["samples"]), vector["sample_rate_hz"], ctypes.byref(c_config), ctypes.byref(actual))
            if status != 0:
                raise AssertionError(f"{vector['name']}: C status {status}")
            for field in FLOAT_FIELDS:
                error = abs(float(getattr(actual, field)) - float(expected[field]))
                max_abs_error = max(max_abs_error, error)
                max_abs_error_by_field[field] = max(max_abs_error_by_field[field], error)
                scale = max(1.0, abs(float(expected[field])))
                if error > 2.0e-5 * scale:
                    raise AssertionError(f"{vector['name']} {field}: error={error}")
                comparisons += 1
            for field in COUNT_FIELDS:
                if int(getattr(actual, field)) != int(expected[field]):
                    raise AssertionError(f"{vector['name']} {field}: count mismatch")
                comparisons += 1

        sentinel = CResult()
        sentinel.duty_ratio = 123.0
        config = CConfig(0, 0.5, 0.05, 1.0e-6, 0.0, 1.0)
        nan_samples = (ctypes.c_float * 5)(0.0, 1.0, math.nan, 0.0, 1.0)
        status = process(nan_samples, 5, 1000.0, ctypes.byref(config), ctypes.byref(sentinel))
        if status != 6 or sentinel.duty_ratio != 123.0:
            raise AssertionError("NaN/error handling or unchanged-result contract failed")
        status = process(None, 5, 1000.0, ctypes.byref(config), ctypes.byref(sentinel))
        if status != 1:
            raise AssertionError("NULL input handling failed")
        short_samples = (ctypes.c_float * 2)(0.0, 1.0)
        if process(short_samples, 2, 1000.0, ctypes.byref(config), ctypes.byref(sentinel)) != 2:
            raise AssertionError("insufficient-data handling failed")
        constant_samples = (ctypes.c_float * 8)(*([1.0] * 8))
        if process(constant_samples, 8, 1000.0, ctypes.byref(config), ctypes.byref(sentinel)) != 5:
            raise AssertionError("constant/no-feature handling failed")
        config.level_mode = 1
        config.low_level = 1.0
        config.high_level = 1.0
        if process(sample_array, len(vector["samples"]), 1000.0, ctypes.byref(config), ctypes.byref(sentinel)) != 3:
            raise AssertionError("explicit-level range handling failed")
        config.level_mode = 99
        if process(sample_array, len(vector["samples"]), 1000.0, ctypes.byref(config), ctypes.byref(sentinel)) != 1:
            raise AssertionError("invalid level-mode handling failed")
        if os.name == "nt":
            handle = library._handle
            del process
            free_library = ctypes.windll.kernel32.FreeLibrary
            free_library.argtypes = [ctypes.c_void_p]
            free_library.restype = ctypes.c_int
            if free_library(ctypes.c_void_p(handle)) == 0:
                raise OSError("FreeLibrary failed for temporary duty DLL")
            library._handle = 0

    print(json.dumps({
        "status": "PASS",
        "vectors": len(vectors),
        "field_comparisons": comparisons,
        "max_absolute_error": max_abs_error,
        "max_absolute_error_by_field": max_abs_error_by_field,
        "edge_checks": [
            "NaN", "result_unchanged_on_error", "NULL", "insufficient_data",
            "constant_signal", "explicit_level_range", "invalid_level_mode"
        ],
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
