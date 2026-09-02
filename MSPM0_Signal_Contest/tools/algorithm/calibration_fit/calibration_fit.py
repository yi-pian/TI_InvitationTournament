#!/usr/bin/env python3
"""Fit calibration CSV data and emit metrics plus MCU-safe calibration.h."""

from __future__ import annotations

import argparse
import csv
import json
import math
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable


@dataclass
class Model:
    name: str
    complexity: int
    params: dict
    predict: Callable[[float], float]


def solve(matrix: list[list[float]], vector: list[float]) -> list[float]:
    n = len(vector)
    a = [row[:] + [vector[i]] for i, row in enumerate(matrix)]
    for col in range(n):
        pivot = max(range(col, n), key=lambda row: abs(a[row][col]))
        if abs(a[pivot][col]) < 1e-15:
            raise ValueError("singular fit matrix")
        a[col], a[pivot] = a[pivot], a[col]
        divisor = a[col][col]
        a[col] = [value / divisor for value in a[col]]
        for row in range(n):
            if row == col:
                continue
            factor = a[row][col]
            a[row] = [a[row][j] - factor * a[col][j] for j in range(n + 1)]
    return [a[i][n] for i in range(n)]


def polyfit(points: list[tuple[float, float]], degree: int) -> Model:
    order = degree + 1
    sums = [sum(x ** power for x, _ in points) for power in range(2 * degree + 1)]
    matrix = [[sums[row + col] for col in range(order)] for row in range(order)]
    vector = [sum(y * (x ** power) for x, y in points) for power in range(order)]
    coeff = solve(matrix, vector)
    return Model(
        ("linear", "quadratic", "cubic")[degree - 1],
        order,
        {"coefficients_ascending": coeff},
        lambda x, c=coeff: sum(value * (x ** i) for i, value in enumerate(c)),
    )


def transformed_fit(points: list[tuple[float, float]], kind: str) -> Model:
    if kind == "exponential":
        if any(y <= 0.0 for _, y in points):
            raise ValueError("exponential fit requires y > 0")
        base = polyfit([(x, math.log(y)) for x, y in points], 1)
        intercept, slope = base.params["coefficients_ascending"]
        scale = math.exp(intercept)
        return Model(kind, 2, {"a": scale, "b": slope},
                     lambda x, a=scale, b=slope: a * math.exp(b * x))
    if any(x <= 0.0 for x, _ in points):
        raise ValueError("logarithmic fit requires x > 0")
    base = polyfit([(math.log(x), y) for x, y in points], 1)
    offset, scale = base.params["coefficients_ascending"]
    return Model(kind, 2, {"a": scale, "b": offset},
                 lambda x, a=scale, b=offset: a * math.log(x) + b)


def unique_sorted(points: list[tuple[float, float]]) -> list[tuple[float, float]]:
    buckets: dict[float, list[float]] = {}
    for x, y in points:
        buckets.setdefault(x, []).append(y)
    return [(x, sum(values) / len(values)) for x, values in sorted(buckets.items())]


def table_model(points: list[tuple[float, float]], name: str, max_knots: int | None) -> Model:
    table = unique_sorted(points)
    if len(table) < 2:
        raise ValueError("table fit requires at least two distinct x values")
    if max_knots and len(table) > max_knots:
        indices = sorted({round(i * (len(table) - 1) / (max_knots - 1))
                          for i in range(max_knots)})
        table = [table[index] for index in indices]

    def predict(x: float, knots=table) -> float:
        if x <= knots[0][0]:
            left, right = knots[0], knots[1]
        elif x >= knots[-1][0]:
            left, right = knots[-2], knots[-1]
        else:
            left, right = knots[0], knots[1]
            for index in range(1, len(knots)):
                if x <= knots[index][0]:
                    left, right = knots[index - 1], knots[index]
                    break
        ratio = (x - left[0]) / (right[0] - left[0])
        return left[1] + ratio * (right[1] - left[1])

    return Model(name, len(table), {"knots": table}, predict)


def metrics(model: Model, points: list[tuple[float, float]]) -> dict:
    errors = [model.predict(x) - y for x, y in points]
    mean_y = sum(y for _, y in points) / len(points)
    ss_res = sum(error * error for error in errors)
    ss_tot = sum((y - mean_y) ** 2 for _, y in points)
    relative = [abs(error / y) for error, (_, y) in zip(errors, points) if abs(y) > 1e-15]
    return {
        "r2": 1.0 - ss_res / ss_tot if ss_tot > 0.0 else (1.0 if ss_res == 0.0 else None),
        "rmse": math.sqrt(ss_res / len(points)),
        "max_absolute_error": max(abs(error) for error in errors),
        "max_relative_error": max(relative) if relative else None,
        "relative_error_points": len(relative),
    }


def load_points(path: Path) -> list[tuple[float, float]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        if not reader.fieldnames or "x" not in reader.fieldnames or "y" not in reader.fieldnames:
            raise ValueError("CSV must contain headers x,y")
        points = [(float(row["x"]), float(row["y"])) for row in reader]
    if len(points) < 4:
        raise ValueError("at least four x,y rows are required")
    if any(not math.isfinite(value) for point in points for value in point):
        raise ValueError("CSV contains non-finite values")
    if len({x for x, _ in points}) < 4:
        raise ValueError("at least four distinct x values are required")
    return sorted(points)


def fit_candidates(points: list[tuple[float, float]], piecewise_knots: int) -> tuple[list[Model], list[str]]:
    candidates: list[Model] = []
    skipped: list[str] = []
    builders = [
        ("linear", lambda: polyfit(points, 1)),
        ("quadratic", lambda: polyfit(points, 2)),
        ("cubic", lambda: polyfit(points, 3)),
        ("exponential", lambda: transformed_fit(points, "exponential")),
        ("logarithmic", lambda: transformed_fit(points, "logarithmic")),
        ("piecewise_linear", lambda: table_model(points, "piecewise_linear", piecewise_knots)),
        ("lut_interpolation", lambda: table_model(points, "lut_interpolation", None)),
    ]
    for name, builder in builders:
        try:
            candidates.append(builder())
        except (ValueError, OverflowError) as exc:
            skipped.append(f"{name}: {exc}")
    return candidates, skipped


def passes(metric: dict, args) -> bool:
    checks = []
    if args.min_r2 is not None:
        checks.append(metric["r2"] is not None and metric["r2"] >= args.min_r2)
    if args.max_rmse is not None:
        checks.append(metric["rmse"] <= args.max_rmse)
    if args.max_abs_error is not None:
        checks.append(metric["max_absolute_error"] <= args.max_abs_error)
    if args.max_rel_error is not None:
        checks.append(metric["max_relative_error"] is not None and
                      metric["max_relative_error"] <= args.max_rel_error)
    return all(checks) if checks else False


def select_model(rows: list[dict], y_range: float, args) -> dict:
    explicit = any(value is not None for value in
                   (args.min_r2, args.max_rmse, args.max_abs_error, args.max_rel_error))
    if explicit:
        passing = [row for row in rows if passes(row["selection_metrics"], args)]
        if passing:
            return min(passing, key=lambda row: (row["complexity"], row["selection_metrics"]["rmse"]))
    scale = max(abs(y_range), 1e-12)
    implicit = [row for row in rows
                if row["selection_metrics"]["rmse"] <= 0.01 * scale and
                row["selection_metrics"]["max_absolute_error"] <= 0.03 * scale]
    if implicit:
        return min(implicit, key=lambda row: (row["complexity"], row["selection_metrics"]["rmse"]))
    return min(rows, key=lambda row: (row["selection_metrics"]["rmse"] / scale +
                                      0.002 * row["complexity"], row["complexity"]))


def c_float(value: float) -> str:
    text = f"{value:.9g}"
    if not any(marker in text for marker in (".", "e", "E")):
        text += ".0"
    return text + "f"


def emit_header(model: Model, x_min: float, x_max: float) -> str:
    head = [
        "#ifndef CALIBRATION_H", "#define CALIBRATION_H", "",
        "/* Generated by calibration_fit.py. Validate on independent data before board use. */",
        "#include <stddef.h>", "#include <stdint.h>",
    ]
    if model.name in {"exponential", "logarithmic"}:
        head.append("#include <math.h>")
    head.extend(["", f"#define CALIBRATION_MODEL_{model.name.upper()} (1)",
                 f"#define CALIBRATION_X_MIN ({c_float(x_min)})",
                 f"#define CALIBRATION_X_MAX ({c_float(x_max)})", ""])

    if model.name in {"linear", "quadratic", "cubic"}:
        coeff = model.params["coefficients_ascending"]
        expression = c_float(coeff[-1])
        for value in reversed(coeff[:-1]):
            expression = f"({expression} * x + {c_float(value)})"
        body = ["static inline int Calibration_Apply(float x, float *y)", "{",
                "    if ((y == NULL) || (x < CALIBRATION_X_MIN) || (x > CALIBRATION_X_MAX)) return 0;",
                f"    *y = {expression};", "    return 1;", "}"]
    elif model.name == "exponential":
        body = ["static inline int Calibration_Apply(float x, float *y)", "{",
                "    if ((y == NULL) || (x < CALIBRATION_X_MIN) || (x > CALIBRATION_X_MAX)) return 0;",
                f"    *y = {c_float(model.params['a'])} * expf({c_float(model.params['b'])} * x);",
                "    return 1;", "}"]
    elif model.name == "logarithmic":
        body = ["static inline int Calibration_Apply(float x, float *y)", "{",
                "    if ((y == NULL) || (x <= 0.0f) || (x < CALIBRATION_X_MIN) || (x > CALIBRATION_X_MAX)) return 0;",
                f"    *y = {c_float(model.params['a'])} * logf(x) + {c_float(model.params['b'])};",
                "    return 1;", "}"]
    else:
        knots = model.params["knots"]
        x_values = ", ".join(c_float(x) for x, _ in knots)
        y_values = ", ".join(c_float(y) for _, y in knots)
        body = [f"#define CALIBRATION_POINT_COUNT ({len(knots)}U)",
                f"static const float g_calibration_x[CALIBRATION_POINT_COUNT] = {{{x_values}}};",
                f"static const float g_calibration_y[CALIBRATION_POINT_COUNT] = {{{y_values}}};", "",
                "static inline int Calibration_Apply(float x, float *y)", "{",
                "    uint32_t i;",
                "    if ((y == NULL) || (x < CALIBRATION_X_MIN) || (x > CALIBRATION_X_MAX)) return 0;",
                "    for (i = 1U; i < CALIBRATION_POINT_COUNT; ++i) {",
                "        if (x <= g_calibration_x[i]) {",
                "            float ratio = (x - g_calibration_x[i - 1U]) /",
                "                          (g_calibration_x[i] - g_calibration_x[i - 1U]);",
                "            *y = g_calibration_y[i - 1U] + ratio *",
                "                 (g_calibration_y[i] - g_calibration_y[i - 1U]);",
                "            return 1;", "        }", "    }", "    return 0;", "}"]
    return "\n".join(head + body + ["", "#endif /* CALIBRATION_H */", ""])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", type=Path, help="CSV with x,y headers")
    parser.add_argument("--output-dir", type=Path, default=Path("calibration_output"))
    parser.add_argument("--piecewise-knots", type=int, default=8)
    parser.add_argument("--min-r2", type=float)
    parser.add_argument("--max-rmse", type=float)
    parser.add_argument("--max-abs-error", type=float)
    parser.add_argument("--max-rel-error", type=float,
                        help="ratio, e.g. 0.01 for 1 percent")
    args = parser.parse_args()
    if args.piecewise_knots < 2:
        parser.error("--piecewise-knots must be >= 2")

    try:
        all_points = load_points(args.csv)
        if len(all_points) >= 8:
            validation = [point for index, point in enumerate(all_points) if index % 4 == 0]
            training = [point for index, point in enumerate(all_points) if index % 4 != 0]
            selection_kind = "deterministic_holdout_every_fourth_sorted_point"
        else:
            training = validation = all_points
            selection_kind = "in_sample_too_few_points_for_holdout"

        candidates, skipped = fit_candidates(training, args.piecewise_knots)
        if not candidates:
            raise ValueError("no model could be fitted")
        rows = []
        valid_candidates = []
        for model in candidates:
            try:
                selection_metrics = metrics(model, validation)
            except (ValueError, OverflowError, ZeroDivisionError) as exc:
                skipped.append(f"{model.name} validation: {exc}")
                continue
            rows.append({"name": model.name, "complexity": model.complexity,
                         "selection_metrics": selection_metrics})
            valid_candidates.append(model)
        if not rows:
            raise ValueError("no model remained valid on selection data")
        y_values = [y for _, y in all_points]
        selected_row = select_model(rows, max(y_values) - min(y_values), args)

        final_models, final_skipped = fit_candidates(all_points, args.piecewise_knots)
        final_model = next(model for model in final_models if model.name == selected_row["name"])
        final_metrics = metrics(final_model, all_points)
        report = {
            "status": "PC_VERIFIED",
            "input": str(args.csv.resolve()),
            "point_count": len(all_points),
            "selection": selection_kind,
            "selection_rows": rows,
            "skipped_candidates": sorted(set(skipped + final_skipped)),
            "recommended_model": final_model.name,
            "recommended_complexity": final_model.complexity,
            "recommended_parameters": final_model.params,
            "full_data_metrics": final_metrics,
            "x_range": [all_points[0][0], all_points[-1][0]],
            "board": "NOT_RUN",
        }
        args.output_dir.mkdir(parents=True, exist_ok=True)
        (args.output_dir / "calibration_report.json").write_text(
            json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        (args.output_dir / "calibration.h").write_text(
            emit_header(final_model, all_points[0][0], all_points[-1][0]), encoding="utf-8")
        print(json.dumps(report, ensure_ascii=False, indent=2))
        print(f"WROTE {args.output_dir / 'calibration_report.json'}")
        print(f"WROTE {args.output_dir / 'calibration.h'}")
        return 0
    except (OSError, ValueError, OverflowError, ZeroDivisionError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
