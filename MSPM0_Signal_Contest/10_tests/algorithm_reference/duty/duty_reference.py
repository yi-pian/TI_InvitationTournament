"""Independent Python reference for the clean duty-cycle reimplementation."""

from __future__ import annotations

from dataclasses import dataclass
import math
import random
from typing import Iterable


INVALID_ARGUMENT = "SIGNAL_ALGORITHM_INVALID_ARGUMENT"
INSUFFICIENT_DATA = "SIGNAL_ALGORITHM_INSUFFICIENT_DATA"
OUT_OF_RANGE = "SIGNAL_ALGORITHM_OUT_OF_RANGE"
NO_FEATURE = "SIGNAL_ALGORITHM_NO_FEATURE"
NUMERIC_ERROR = "SIGNAL_ALGORITHM_NUMERIC_ERROR"


class DutyReferenceError(ValueError):
    def __init__(self, status: str) -> None:
        super().__init__(status)
        self.status = status


@dataclass(frozen=True)
class DutyConfig:
    level_mode: str = "AUTO_MIN_MAX"
    threshold_ratio: float = 0.5
    hysteresis_ratio: float = 0.05
    min_amplitude: float = 1.0e-6
    low_level: float = 0.0
    high_level: float = 1.0


def _crossing(index: int, previous: float, current: float, threshold: float) -> float:
    delta = current - previous
    if delta == 0.0 or not math.isfinite(delta):
        raise DutyReferenceError(NUMERIC_ERROR)
    position = (index - 1) + (threshold - previous) / delta
    if not math.isfinite(position):
        raise DutyReferenceError(NUMERIC_ERROR)
    return position


def measure_duty(
    samples: Iterable[float],
    sample_rate_hz: float,
    config: DutyConfig | None = None,
) -> dict[str, float | int]:
    """Measure positive duty from complete rising-falling-rising cycles."""

    cfg = config or DutyConfig()
    values = [float(value) for value in samples]
    if len(values) < 3:
        raise DutyReferenceError(INSUFFICIENT_DATA)
    if not math.isfinite(sample_rate_hz) or sample_rate_hz <= 0.0:
        raise DutyReferenceError(NUMERIC_ERROR)
    numeric_config = (
        cfg.threshold_ratio,
        cfg.hysteresis_ratio,
        cfg.min_amplitude,
        cfg.low_level,
        cfg.high_level,
    )
    if not all(math.isfinite(value) for value in numeric_config):
        raise DutyReferenceError(NUMERIC_ERROR)
    if cfg.level_mode not in {"AUTO_MIN_MAX", "EXPLICIT"}:
        raise DutyReferenceError(INVALID_ARGUMENT)
    if not (0.0 < cfg.threshold_ratio < 1.0):
        raise DutyReferenceError(OUT_OF_RANGE)
    if cfg.hysteresis_ratio < 0.0 or cfg.hysteresis_ratio >= min(
        cfg.threshold_ratio, 1.0 - cfg.threshold_ratio
    ):
        raise DutyReferenceError(OUT_OF_RANGE)
    if cfg.min_amplitude < 0.0:
        raise DutyReferenceError(OUT_OF_RANGE)
    if not all(math.isfinite(value) for value in values):
        raise DutyReferenceError(NUMERIC_ERROR)

    if cfg.level_mode == "AUTO_MIN_MAX":
        low_level = min(values)
        high_level = max(values)
    else:
        low_level = cfg.low_level
        high_level = cfg.high_level
    if high_level <= low_level:
        if cfg.level_mode == "AUTO_MIN_MAX":
            raise DutyReferenceError(NO_FEATURE)
        raise DutyReferenceError(OUT_OF_RANGE)

    amplitude = high_level - low_level
    if amplitude <= cfg.min_amplitude:
        raise DutyReferenceError(NO_FEATURE)

    threshold = low_level + cfg.threshold_ratio * amplitude
    lower_guard = low_level + (cfg.threshold_ratio - cfg.hysteresis_ratio) * amplitude
    upper_guard = low_level + (cfg.threshold_ratio + cfg.hysteresis_ratio) * amplitude

    state = "UNKNOWN"
    if values[0] <= lower_guard:
        state = "LOW"
    elif values[0] >= upper_guard:
        state = "HIGH"

    candidate: float | None = None
    last_rise: float | None = None
    fall_after_rise: float | None = None
    sum_period_samples = 0.0
    sum_high_samples = 0.0
    valid_cycles = 0
    rising_edges = 0
    falling_edges = 0

    for index in range(1, len(values)):
        previous = values[index - 1]
        current = values[index]

        if state == "UNKNOWN":
            if current <= lower_guard:
                state = "LOW"
            elif current >= upper_guard:
                state = "HIGH"
            continue

        if state == "LOW":
            if candidate is None and previous < threshold <= current:
                candidate = _crossing(index, previous, current, threshold)
            if candidate is not None:
                if current >= upper_guard:
                    rise = candidate
                    rising_edges += 1
                    if (
                        last_rise is not None
                        and fall_after_rise is not None
                        and last_rise < fall_after_rise < rise
                    ):
                        period = rise - last_rise
                        high_width = fall_after_rise - last_rise
                        if 0.0 < high_width < period:
                            sum_period_samples += period
                            sum_high_samples += high_width
                            valid_cycles += 1
                    last_rise = rise
                    fall_after_rise = None
                    candidate = None
                    state = "HIGH"
                elif current <= lower_guard:
                    candidate = None
        else:  # HIGH
            if candidate is None and previous > threshold >= current:
                candidate = _crossing(index, previous, current, threshold)
            if candidate is not None:
                if current <= lower_guard:
                    fall = candidate
                    falling_edges += 1
                    if last_rise is not None and fall > last_rise and fall_after_rise is None:
                        fall_after_rise = fall
                    candidate = None
                    state = "LOW"
                elif current >= upper_guard:
                    candidate = None

    if valid_cycles == 0 or sum_period_samples <= 0.0:
        raise DutyReferenceError(NO_FEATURE)

    mean_period_samples = sum_period_samples / valid_cycles
    mean_high_samples = sum_high_samples / valid_cycles
    mean_low_samples = mean_period_samples - mean_high_samples
    duty_ratio = sum_high_samples / sum_period_samples
    period_s = mean_period_samples / sample_rate_hz
    result = {
        "duty_ratio": duty_ratio,
        "duty_percent": 100.0 * duty_ratio,
        "period_s": period_s,
        "frequency_hz": 1.0 / period_s,
        "high_width_s": mean_high_samples / sample_rate_hz,
        "low_width_s": mean_low_samples / sample_rate_hz,
        "low_level": low_level,
        "high_level": high_level,
        "threshold_level": threshold,
        "valid_cycle_count": valid_cycles,
        "rising_edge_count": rising_edges,
        "falling_edge_count": falling_edges,
    }
    if not all(math.isfinite(float(value)) for value in result.values()):
        raise DutyReferenceError(NUMERIC_ERROR)
    return result


def synthesize_trapezoid(
    count: int,
    period_samples: float,
    duty_ratio: float,
    amplitude: float = 1.0,
    offset: float = 0.0,
    phase_samples: float = 0.0,
    transition_samples: float = 4.0,
    noise_rms: float = 0.0,
    seed: int = 0,
) -> list[float]:
    """Generate a periodic trapezoid whose 50% crossings define exact duty."""

    if count < 1 or period_samples <= 0.0 or not 0.0 < duty_ratio < 1.0:
        raise ValueError("invalid synthetic waveform geometry")
    high_center = duty_ratio * period_samples
    if transition_samples <= 0.0 or transition_samples >= 2.0 * min(
        high_center, period_samples - high_center
    ):
        raise ValueError("transition is too long for the requested pulse")
    rng = random.Random(seed)
    values: list[float] = []
    half_transition = 0.5 * transition_samples
    for index in range(count):
        phase = (index - phase_samples) % period_samples
        if phase < half_transition:
            normalized = 0.5 + phase / transition_samples
        elif phase < high_center - half_transition:
            normalized = 1.0
        elif phase < high_center + half_transition:
            normalized = 0.5 - (phase - high_center) / transition_samples
        elif phase < period_samples - half_transition:
            normalized = 0.0
        else:
            normalized = 0.5 + (phase - period_samples) / transition_samples
        noise = rng.gauss(0.0, noise_rms) if noise_rms > 0.0 else 0.0
        values.append(offset + amplitude * normalized + noise)
    return values
