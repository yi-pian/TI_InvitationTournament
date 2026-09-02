#!/usr/bin/env python3
"""Plot INDEX,ADC_RAW UART data and print simple acceptance statistics."""

from __future__ import annotations

import argparse
import csv
import html
import webbrowser
from pathlib import Path
from statistics import fmean


def read_adc_csv(path: Path) -> tuple[list[int], list[int]]:
    indices: list[int] = []
    samples: list[int] = []

    with path.open("r", encoding="utf-8-sig", newline="") as csv_file:
        reader = csv.reader(csv_file)
        try:
            header = next(reader)
        except StopIteration as exc:
            raise ValueError("CSV is empty") from exc

        normalized = [column.strip().upper() for column in header]
        if normalized != ["INDEX", "ADC_RAW"]:
            raise ValueError(
                "expected header INDEX,ADC_RAW; disable terminal timestamps "
                f"and recapture (got {header!r})"
            )

        for line_number, row in enumerate(reader, start=2):
            if not row or all(not field.strip() for field in row):
                continue
            if len(row) != 2:
                raise ValueError(f"line {line_number}: expected 2 columns")
            index = int(row[0].strip())
            sample = int(row[1].strip())
            if not 0 <= sample <= 4095:
                raise ValueError(
                    f"line {line_number}: ADC_RAW {sample} is outside 0..4095"
                )
            indices.append(index)
            samples.append(sample)

    if not samples:
        raise ValueError("CSV contains no samples")
    if indices != list(range(len(indices))):
        raise ValueError("INDEX must be continuous and start at 0")
    return indices, samples


def write_svg(path: Path, indices: list[int], samples: list[int]) -> None:
    """Write a dependency-free ADC Raw vs Sample Index line plot."""
    width = 1200
    height = 600
    left = 85
    right = 30
    top = 45
    bottom = 70
    plot_width = width - left - right
    plot_height = height - top - bottom

    def x_position(index: int) -> float:
        if len(indices) == 1:
            return left + plot_width / 2
        return left + (index / (len(indices) - 1)) * plot_width

    def y_position(sample: int) -> float:
        return top + (1.0 - sample / 4095.0) * plot_height

    points = " ".join(
        f"{x_position(index):.2f},{y_position(sample):.2f}"
        for index, sample in zip(indices, samples)
    )
    title = html.escape("MSPM0 ADC Raw Capture")
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" '
        f'height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="white"/>',
        '<g font-family="Segoe UI, Arial, sans-serif" font-size="14" '
        'fill="#1f2937">',
        f'<text x="{width / 2}" y="25" text-anchor="middle" '
        f'font-size="20">{title}</text>',
    ]

    for tick in (0, 1024, 2048, 3072, 4095):
        y = y_position(tick)
        lines.append(
            f'<line x1="{left}" y1="{y:.2f}" x2="{left + plot_width}" '
            f'y2="{y:.2f}" stroke="#d1d5db" stroke-width="1"/>'
        )
        lines.append(
            f'<text x="{left - 10}" y="{y + 5:.2f}" '
            f'text-anchor="end">{tick}</text>'
        )

    x_ticks = sorted({0, (len(indices) - 1) // 4, (len(indices) - 1) // 2,
                      ((len(indices) - 1) * 3) // 4, len(indices) - 1})
    for tick in x_ticks:
        x = x_position(tick)
        lines.append(
            f'<line x1="{x:.2f}" y1="{top}" x2="{x:.2f}" '
            f'y2="{top + plot_height}" stroke="#e5e7eb" stroke-width="1"/>'
        )
        lines.append(
            f'<text x="{x:.2f}" y="{top + plot_height + 25}" '
            f'text-anchor="middle">{tick}</text>'
        )

    lines.extend(
        [
            f'<rect x="{left}" y="{top}" width="{plot_width}" '
            f'height="{plot_height}" fill="none" stroke="#374151"/>',
            f'<polyline points="{points}" fill="none" stroke="#0b74c9" '
            'stroke-width="1.5" stroke-linejoin="round"/>',
            f'<text x="{left + plot_width / 2}" y="{height - 18}" '
            'text-anchor="middle">Sample Index</text>',
            f'<text x="20" y="{top + plot_height / 2}" '
            'text-anchor="middle" '
            f'transform="rotate(-90 20 {top + plot_height / 2})">'
            'ADC Raw (12-bit)</text>',
            '</g>',
            '</svg>',
        ]
    )
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Plot an MSPM0 ADC UART CSV capture."
    )
    parser.add_argument("csv_file", type=Path, help="captured INDEX,ADC_RAW CSV")
    parser.add_argument(
        "--output", type=Path, help="SVG output path (default: <csv>_plot.svg)"
    )
    parser.add_argument(
        "--no-show", action="store_true", help="do not open an interactive window"
    )
    args = parser.parse_args()

    indices, samples = read_adc_csv(args.csv_file)
    print(f"sample count: {len(samples)}")
    print(f"raw min:      {min(samples)}")
    print(f"raw max:      {max(samples)}")
    print(f"raw mean:     {fmean(samples):.3f}")

    output_path = args.output or args.csv_file.with_name(
        f"{args.csv_file.stem}_plot.svg"
    )
    if output_path.suffix.lower() != ".svg":
        raise SystemExit("--output must use the .svg extension")
    write_svg(output_path, indices, samples)
    print(f"plot saved:   {output_path}")
    if not args.no_show:
        webbrowser.open(output_path.resolve().as_uri())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
