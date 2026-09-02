#!/usr/bin/env python3
"""Validate Measurement Recipe structure, links, target coverage, and Signal API symbols."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


SIGNAL_API_RE = re.compile(r"\bSignal[A-Z][A-Za-z0-9_]+")
LINK_RE = re.compile(r"\[[^\]]+\]\(([^)#]+)(?:#[^)]+)?\)")
REQUIRED_TARGETS = (
    "频率", "周期", "Vpp", "RMS", "AC RMS", "DC Offset", "相位差", "时间延迟",
    "占空比", "脉宽", "上升时间", "下降时间", "Slew Rate", "增益 dB", "-3 dB",
    "UGBW", "THD", "SNR", "SINAD", "振铃", "过冲", "自动量程", "自动增益",
    "ADC", "DAC", "VGA", "PGA", "前端频响", "通道延时", "传感器多点标定", "数据拟合",
)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def iter_source_files(root: Path):
    for path in root.rglob("*"):
        if path.suffix.lower() in {".c", ".h"} and "build" not in path.parts:
            yield path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--algorithm-root", type=Path,
                        default=Path(__file__).resolve().parents[3])
    parser.add_argument("--json", type=Path, help="optional JSON report path")
    args = parser.parse_args()

    root = args.algorithm_root.resolve()
    docs = root / "00_docs"
    recipe_root = docs / "measurement_recipes"
    index_path = docs / "MEASUREMENT_RECIPE_INDEX.md"
    errors: list[str] = []
    checked_links = 0
    api_refs: set[str] = set()

    recipes = sorted(p for p in recipe_root.glob("*.md") if p.name != "README.md")
    if not recipes:
        errors.append("no Measurement Recipe files found")

    for path in recipes:
        text = read_text(path)
        if "## 20 项执行契约" not in text:
            errors.append(f"{path.name}: missing 20-item contract heading")
        for number in range(1, 21):
            if not re.search(rf"\|\s*{number}\s*\|", text):
                errors.append(f"{path.name}: missing contract item {number}")
        if "Recipe 状态：`DRAFT`" not in text:
            errors.append(f"{path.name}: missing explicit DRAFT status")
        api_refs.update(SIGNAL_API_RE.findall(text))

    for path in docs.rglob("*.md"):
        text = read_text(path)
        for raw_target in LINK_RE.findall(text):
            if re.match(r"^[a-z]+:", raw_target, re.I):
                continue
            target = (path.parent / raw_target).resolve()
            checked_links += 1
            if not target.exists():
                errors.append(f"broken link: {path.relative_to(root)} -> {raw_target}")

    source_text = "\n".join(read_text(path) for path in iter_source_files(root))
    missing_apis = sorted(name for name in api_refs
                          if not re.search(rf"\b{re.escape(name)}\b", source_text))
    errors.extend(f"referenced Signal API not found: {name}" for name in missing_apis)

    index_text = read_text(index_path) if index_path.exists() else ""
    for target in REQUIRED_TARGETS:
        if target.casefold() not in index_text.casefold():
            errors.append(f"recipe index missing target/search term: {target}")

    report = {
        "status": "PASS" if not errors else "FAIL",
        "recipe_count": len(recipes),
        "contract_fields_per_recipe": 20,
        "markdown_links_checked": checked_links,
        "unique_signal_api_references": len(api_refs),
        "required_target_terms": len(REQUIRED_TARGETS),
        "board": "NOT_RUN",
        "errors": errors,
    }
    print(json.dumps(report, ensure_ascii=False, indent=2))
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                             encoding="utf-8")
    return 0 if not errors else 1


if __name__ == "__main__":
    sys.exit(main())
