"""Independent readable reference models for the eight clean reimplementations."""

from __future__ import annotations

import cmath
import math


def three_dft_bins(size: int, peak: int, fractional_bin: float) -> list[complex]:
    tone_bin = peak + fractional_bin
    return [
        sum(cmath.exp(2j * math.pi * (tone_bin - bin_index) * n / size)
            for n in range(size))
        for bin_index in (peak - 1, peak, peak + 1)
    ]


def jacobsen(bins: list[complex]) -> float:
    left, center, right = bins
    return ((left - right) / (2 * center - left - right)).real


def quinn_second(bins: list[complex]) -> float:
    left, center, right = bins
    beta_minus = (left / center).real
    beta_plus = (right / center).real
    delta_minus = beta_minus / (1.0 - beta_minus)
    delta_plus = -beta_plus / (1.0 - beta_plus)

    def tau(square: float) -> float:
        return (0.25 * math.log(3 * square * square + 6 * square + 1)
                - math.sqrt(6) / 24
                * math.log((square + 1 - math.sqrt(2 / 3))
                           / (square + 1 + math.sqrt(2 / 3))))

    return ((delta_minus + delta_plus) / 2
            + tau(delta_plus * delta_plus)
            - tau(delta_minus * delta_minus))


def macleod(bins: list[complex]) -> float:
    left, center, right = bins
    aligned = [(value * center.conjugate()).real for value in bins]
    gamma = (aligned[0] - aligned[2]) / (
        2 * aligned[1] + aligned[0] + aligned[2])
    if abs(gamma) <= 1e-15:
        return 0.0
    return 2 * gamma / (1 + math.sqrt(1 + 8 * gamma * gamma))


def coherent_nearest(desired: float, sample_rate: float, count: int,
                     minimum: int, maximum: int, coprime: bool) -> dict:
    candidates = [cycles for cycles in range(minimum, maximum + 1)
                  if not coprime or math.gcd(cycles, count) == 1]
    cycles = min(candidates,
                 key=lambda item: (abs(item * sample_rate / count - desired), item))
    frequency = cycles * sample_rate / count
    error = frequency - desired
    return {
        "cycles_per_record": cycles,
        "samples_per_record": count,
        "cycle_sample_gcd": math.gcd(cycles, count),
        "coherent_frequency_hz": frequency,
        "frequency_error_hz": error,
        "absolute_error_hz": abs(error),
        "relative_error_ppm": 1e6 * error / desired,
    }


def wrap_degrees(value: float) -> float:
    while value > 180:
        value -= 360
    while value <= -180:
        value += 360
    return value


def frequency_response_correct(table: list[dict], frequency: float,
                               measured_gain: float, measured_phase: float,
                               log_frequency: bool, clamp: bool) -> dict:
    if frequency < table[0]["frequency_hz"]:
        if not clamp:
            raise ValueError("below table")
        lower = upper = 0
        fraction = 0.0
    elif frequency > table[-1]["frequency_hz"]:
        if not clamp:
            raise ValueError("above table")
        lower = upper = len(table) - 1
        fraction = 0.0
    elif frequency == table[0]["frequency_hz"]:
        lower = upper = 0
        fraction = 0.0
    elif frequency == table[-1]["frequency_hz"]:
        lower = upper = len(table) - 1
        fraction = 0.0
    else:
        upper = next(i for i, point in enumerate(table)
                     if point["frequency_hz"] >= frequency)
        lower = upper - 1
        if log_frequency:
            fraction = ((math.log(frequency) - math.log(table[lower]["frequency_hz"]))
                        / (math.log(table[upper]["frequency_hz"])
                           - math.log(table[lower]["frequency_hz"])))
        else:
            fraction = ((frequency - table[lower]["frequency_hz"])
                        / (table[upper]["frequency_hz"]
                           - table[lower]["frequency_hz"]))
    gain = (table[lower]["gain_correction_linear"] + fraction
            * (table[upper]["gain_correction_linear"]
               - table[lower]["gain_correction_linear"]))
    phase_delta = wrap_degrees(table[upper]["phase_correction_deg"]
                               - table[lower]["phase_correction_deg"])
    phase = wrap_degrees(table[lower]["phase_correction_deg"]
                         + fraction * phase_delta)
    return {
        "corrected_gain_linear": measured_gain * gain,
        "corrected_phase_deg": wrap_degrees(measured_phase + phase),
        "applied_gain_correction_linear": gain,
        "applied_phase_correction_deg": phase,
        "interpolation_fraction": fraction,
        "lower_index": lower,
        "upper_index": upper,
    }


def czt_unit_circle_real(samples: list[float], sample_rate: float,
                         start_frequency: float, step: float,
                         output_count: int) -> list[complex]:
    return [
        sum(value * cmath.exp(-2j * math.pi * frequency * n / sample_rate)
            for n, value in enumerate(samples))
        for frequency in (start_frequency + step * k
                          for k in range(output_count))
    ]


def dc_from_raw(raw: list[int], max_code: int, reference: float,
                scale: float, offset: float) -> dict:
    mean_code = sum(raw) / len(raw)
    return {
        "mean_code": mean_code,
        "dc_voltage_v": mean_code * reference * scale / max_code + offset,
    }


def fft_peak(values: list[float], first: int, last: int,
             sample_rate: float, fft_size: int) -> dict:
    peak = max(range(first, last + 1), key=lambda index: (values[index], -index))
    return {"bin": peak, "peak_value": values[peak],
            "frequency_hz": peak * sample_rate / fft_size}
