#!/usr/bin/env python3
"""Compile all eight clean reimplementations and compare C with golden vectors."""

from __future__ import annotations

import ctypes
import json
import math
import os
from pathlib import Path
import subprocess
import tempfile


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
VECTORS = ROOT / "10_tests/algorithm_reference/source_lost_remaining/test_vectors"


class Complex(ctypes.Structure):
    _fields_ = [("real", ctypes.c_float), ("imag", ctypes.c_float)]


class EstimatorResult(ctypes.Structure):
    _fields_ = [("fractional_bin", ctypes.c_float),
                ("interpolated_bin", ctypes.c_float),
                ("frequency_hz", ctypes.c_float)]


class CoherentResult(ctypes.Structure):
    _fields_ = [("cycles_per_record", ctypes.c_uint32),
                ("samples_per_record", ctypes.c_uint32),
                ("cycle_sample_gcd", ctypes.c_uint32),
                ("coherent_frequency_hz", ctypes.c_float),
                ("frequency_error_hz", ctypes.c_float),
                ("absolute_error_hz", ctypes.c_float),
                ("relative_error_ppm", ctypes.c_float)]


class FRCPoint(ctypes.Structure):
    _fields_ = [("frequency_hz", ctypes.c_float),
                ("gain_correction_linear", ctypes.c_float),
                ("phase_correction_deg", ctypes.c_float)]


class FRCResult(ctypes.Structure):
    _fields_ = [("corrected_gain_linear", ctypes.c_float),
                ("corrected_phase_deg", ctypes.c_float),
                ("applied_gain_correction_linear", ctypes.c_float),
                ("applied_phase_correction_deg", ctypes.c_float),
                ("interpolation_fraction", ctypes.c_float),
                ("lower_index", ctypes.c_uint32),
                ("upper_index", ctypes.c_uint32)]


class ADCConfig(ctypes.Structure):
    _fields_ = [("adc_max_code", ctypes.c_uint32),
                ("reference_voltage_v", ctypes.c_float),
                ("input_scale", ctypes.c_float),
                ("offset_voltage_v", ctypes.c_float)]


class DCResult(ctypes.Structure):
    _fields_ = [("mean_code", ctypes.c_float),
                ("dc_voltage_v", ctypes.c_float)]


class FFTPeakResult(ctypes.Structure):
    _fields_ = [("bin", ctypes.c_uint32),
                ("peak_value", ctypes.c_float),
                ("frequency_hz", ctypes.c_float)]


def load_vectors(name: str) -> list[dict]:
    return json.loads((VECTORS / f"{name}.json").read_text(encoding="utf-8"))["vectors"]


def close_library(library: ctypes.CDLL) -> None:
    if os.name == "nt":
        free_library = ctypes.windll.kernel32.FreeLibrary
        free_library.argtypes = [ctypes.c_void_p]
        free_library.restype = ctypes.c_int
        if free_library(ctypes.c_void_p(library._handle)) == 0:
            raise OSError("FreeLibrary failed")
        library._handle = 0


def assert_close(actual: float, expected: float, tolerance: float = 3e-5) -> None:
    error = abs(actual - expected)
    if error > tolerance * max(1.0, abs(expected)):
        raise AssertionError(f"actual={actual} expected={expected} error={error}")


def main() -> int:
    sources = [
        ROOT / "11_legacy_compatibility/algorithms/03_measurement/mean/signal_mean.c",
        ROOT / "03_measurement/dc_measure/signal_dc_measure.c",
        ROOT / "11_legacy_compatibility/algorithms/04_dsp/peak_detect/signal_peak_detect.c",
        ROOT / "04_dsp/fft_peak/signal_fft_peak.c",
        ROOT / "05_precision/jacobsen_interpolation/signal_jacobsen_interpolation.c",
        ROOT / "05_precision/quinn_interpolation/signal_quinn_interpolation.c",
        ROOT / "05_precision/macleod_interpolation/signal_macleod_interpolation.c",
        ROOT / "05_precision/coherent_sampling/signal_coherent_sampling.c",
        ROOT / "05_precision/frequency_response_correction/signal_frequency_response_correction.c",
        ROOT / "05_precision/czt/signal_czt.c",
    ]
    include_dirs = [
        ROOT / "03_measurement/common", ROOT / "11_legacy_compatibility/algorithms/03_measurement/adc_to_voltage",
        ROOT / "11_legacy_compatibility/algorithms/03_measurement/mean", ROOT / "03_measurement/dc_measure",
        ROOT / "11_legacy_compatibility/algorithms/04_dsp/peak_detect", ROOT / "04_dsp/fft_peak",
        ROOT / "05_precision/jacobsen_interpolation",
        ROOT / "05_precision/quinn_interpolation",
        ROOT / "05_precision/macleod_interpolation",
        ROOT / "05_precision/coherent_sampling",
        ROOT / "05_precision/frequency_response_correction",
        ROOT / "05_precision/czt",
    ]
    comparisons = 0
    edge_checks: list[str] = []
    with tempfile.TemporaryDirectory() as directory:
        dll = Path(directory) / "source_lost_remaining.dll"
        command = ["gcc", "-shared", "-O2", "-std=c11", "-Wall", "-Wextra",
                   "-Werror", "-pedantic"]
        command += [f"-I{path}" for path in include_dirs]
        command += [str(path) for path in sources] + ["-lm", "-o", str(dll)]
        subprocess.run(command, check=True)
        library = ctypes.CDLL(str(dll))

        estimators = [
            ("jacobsen_interpolation", "SignalJacobsen_Process", "jacobsen"),
            ("quinn_interpolation", "SignalQuinnSecond_Process", "quinn_second"),
            ("macleod_interpolation", "SignalMacleod_Process", "macleod"),
        ]
        for vector_name, symbol, expected_field in estimators:
            function = getattr(library, symbol)
            function.argtypes = [ctypes.POINTER(Complex), ctypes.c_uint32,
                                 ctypes.c_uint32, ctypes.c_float,
                                 ctypes.c_uint32, ctypes.POINTER(EstimatorResult)]
            function.restype = ctypes.c_int
            for vector in load_vectors(vector_name):
                peak = vector["peak_index"]
                spectrum = (Complex * (peak + 2))()
                for offset, pair in enumerate(vector["bins"]):
                    spectrum[peak - 1 + offset] = Complex(*pair)
                result = EstimatorResult()
                status = function(spectrum, len(spectrum), peak,
                                  vector["sample_rate_hz"], vector["fft_size"],
                                  ctypes.byref(result))
                if status != 0:
                    raise AssertionError(f"{symbol} status={status}")
                expected_delta = vector[expected_field]
                assert_close(result.fractional_bin, expected_delta)
                assert_close(result.interpolated_bin, peak + expected_delta)
                assert_close(result.frequency_hz,
                             (peak + expected_delta) * vector["sample_rate_hz"]
                             / vector["fft_size"])
                comparisons += 3
            sentinel = EstimatorResult(99, 99, 99)
            invalid = (Complex * 3)(Complex(0, 0), Complex(0, 0), Complex(0, 0))
            if function(invalid, 3, 1, 1000, 8, ctypes.byref(sentinel)) != 5:
                raise AssertionError(f"{symbol} zero feature")
            if sentinel.fractional_bin != 99:
                raise AssertionError(f"{symbol} changed result on error")
            edge_checks.append(f"{symbol}:zero_and_unchanged")

        coherent = library.SignalCoherentSampling_FindNearest
        coherent.argtypes = [ctypes.c_float, ctypes.c_float, ctypes.c_uint32,
                             ctypes.c_uint32, ctypes.c_uint32, ctypes.c_bool,
                             ctypes.POINTER(CoherentResult)]
        coherent.restype = ctypes.c_int
        for vector in load_vectors("coherent_sampling"):
            inputs = vector["input"]
            result = CoherentResult()
            status = coherent(inputs["desired_frequency_hz"], inputs["sample_rate_hz"],
                              inputs["sample_count"], inputs["minimum_cycles"],
                              inputs["maximum_cycles"], inputs["require_coprime"],
                              ctypes.byref(result))
            if status != 0:
                raise AssertionError(f"coherent status={status}")
            for field in ("cycles_per_record", "samples_per_record", "cycle_sample_gcd"):
                if getattr(result, field) != vector["expected"][field]:
                    raise AssertionError(field)
                comparisons += 1
            for field in ("coherent_frequency_hz", "frequency_error_hz",
                          "absolute_error_hz", "relative_error_ppm"):
                assert_close(getattr(result, field), vector["expected"][field])
                comparisons += 1
        if coherent(1000, 48000, 1, 1, 1, False, ctypes.byref(CoherentResult())) != 2:
            raise AssertionError("coherent insufficient")
        edge_checks.append("coherent:invalid_record")

        frc = library.SignalFrequencyResponseCorrection_Process
        frc.argtypes = [ctypes.POINTER(FRCPoint), ctypes.c_uint32, ctypes.c_float,
                        ctypes.c_float, ctypes.c_float, ctypes.c_int, ctypes.c_int,
                        ctypes.POINTER(FRCResult)]
        frc.restype = ctypes.c_int
        for vector in load_vectors("frequency_response_correction"):
            table = (FRCPoint * len(vector["table"]))(*[
                FRCPoint(point["frequency_hz"], point["gain_correction_linear"],
                         point["phase_correction_deg"]) for point in vector["table"]])
            result = FRCResult()
            status = frc(table, len(table), vector["frequency_hz"],
                         vector["measured_gain_linear"], vector["measured_phase_deg"],
                         int(vector["log_frequency"]), int(vector["clamp"]),
                         ctypes.byref(result))
            if status != 0:
                raise AssertionError(f"frc status={status}")
            for field in ("corrected_gain_linear", "corrected_phase_deg",
                          "applied_gain_correction_linear",
                          "applied_phase_correction_deg", "interpolation_fraction"):
                assert_close(getattr(result, field), vector["expected"][field])
                comparisons += 1
            for field in ("lower_index", "upper_index"):
                if getattr(result, field) != vector["expected"][field]:
                    raise AssertionError(field)
                comparisons += 1
        bad_table = (FRCPoint * 2)(FRCPoint(100, 1, 0), FRCPoint(90, 1, 0))
        if frc(bad_table, 2, 95, 1, 0, 0, 0, ctypes.byref(FRCResult())) != 3:
            raise AssertionError("frc unsorted")
        edge_checks.append("frequency_response:unsorted_table")

        czt = library.SignalCZT_UnitCircleRealDirect
        czt.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.c_uint32,
                        ctypes.c_float, ctypes.c_float, ctypes.c_float,
                        ctypes.POINTER(Complex), ctypes.c_uint32, ctypes.c_uint32]
        czt.restype = ctypes.c_int
        for vector in load_vectors("czt"):
            samples = (ctypes.c_float * len(vector["samples"]))(*vector["samples"])
            output = (Complex * len(vector["expected"]))()
            status = czt(samples, len(samples), vector["sample_rate_hz"],
                         vector["start_frequency_hz"], vector["frequency_step_hz"],
                         output, len(output), len(output))
            if status != 0:
                raise AssertionError(f"czt status={status}")
            for actual, expected in zip(output, vector["expected"]):
                assert_close(actual.real, expected[0], 2e-4)
                assert_close(actual.imag, expected[1], 2e-4)
                comparisons += 2
        if czt(samples, len(samples), 8000, 0, 100, output, 10, 9) != 4:
            raise AssertionError("czt capacity")
        edge_checks.append("czt:capacity")

        raw_dc = library.SignalDCMeasure_FromRawLinear
        raw_dc.argtypes = [ctypes.POINTER(ctypes.c_uint16), ctypes.c_uint32,
                           ctypes.POINTER(ADCConfig), ctypes.POINTER(DCResult)]
        raw_dc.restype = ctypes.c_int
        for vector in load_vectors("dc_measure"):
            raw = (ctypes.c_uint16 * len(vector["raw"]))(*vector["raw"])
            config = ADCConfig(vector["adc_max_code"], vector["reference_voltage_v"],
                               vector["input_scale"], vector["offset_voltage_v"])
            result = DCResult()
            if raw_dc(raw, len(raw), ctypes.byref(config), ctypes.byref(result)) != 0:
                raise AssertionError("dc status")
            assert_close(result.mean_code, vector["expected"]["mean_code"])
            assert_close(result.dc_voltage_v, vector["expected"]["dc_voltage_v"])
            comparisons += 2
        bad_raw = (ctypes.c_uint16 * 1)(4096)
        config = ADCConfig(4095, 3.3, 1, 0)
        if raw_dc(bad_raw, 1, ctypes.byref(config), ctypes.byref(DCResult())) != 3:
            raise AssertionError("dc out of range")
        edge_checks.append("dc:raw_range")

        peak_fn = library.SignalFFTPeak_Process
        peak_fn.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.c_uint32,
                            ctypes.c_uint32, ctypes.c_uint32, ctypes.c_float,
                            ctypes.c_uint32, ctypes.POINTER(FFTPeakResult)]
        peak_fn.restype = ctypes.c_int
        for vector in load_vectors("fft_peak"):
            values = (ctypes.c_float * len(vector["values"]))(*vector["values"])
            result = FFTPeakResult()
            if peak_fn(values, len(values), vector["first_bin"], vector["last_bin"],
                       vector["sample_rate_hz"], vector["fft_size"],
                       ctypes.byref(result)) != 0:
                raise AssertionError("fft peak status")
            if result.bin != vector["expected"]["bin"]:
                raise AssertionError("fft peak bin")
            assert_close(result.peak_value, vector["expected"]["peak_value"])
            assert_close(result.frequency_hz, vector["expected"]["frequency_hz"])
            comparisons += 3
        if peak_fn(values, len(values), 5, 1, 12000, 12,
                   ctypes.byref(FFTPeakResult())) != 3:
            raise AssertionError("fft peak range")
        edge_checks.append("fft_peak:range")

        close_library(library)
        del library

    print(json.dumps({"status": "PASS", "modules": 8,
                      "field_comparisons": comparisons,
                      "edge_checks": edge_checks}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
