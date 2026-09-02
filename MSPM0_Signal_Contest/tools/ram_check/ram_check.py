#!/usr/bin/env python3
"""Compute logical peak-live RAM and optionally read TI linker SRAM use from a .map."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


TI_SRAM_RE = re.compile(
    r"^\s*SRAM\s+[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)",
    re.MULTILINE,
)


def parse_map(path: Path) -> dict:
    text = path.read_text(encoding="utf-8", errors="replace")
    match = TI_SRAM_RE.search(text)
    if not match:
        raise ValueError("could not find TI linker MEMORY CONFIGURATION SRAM row")
    capacity, used, unused = (int(value, 16) for value in match.groups())
    return {"path": str(path.resolve()), "capacity_bytes": capacity,
            "used_bytes": used, "unused_bytes": unused}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--map", dest="map_path", type=Path)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()

    try:
        data = json.loads(args.manifest.read_text(encoding="utf-8-sig"))
        capacity = int(data["capacity_bytes"])
        fixed = {name: int(data.get(name, 0)) for name in
                 ("globals_bytes", "stack_bytes", "library_workspace_bytes")}
        buffers = data.get("buffers", [])
        if capacity <= 0 or not isinstance(buffers, list):
            raise ValueError("capacity_bytes must be positive and buffers must be a list")

        phases: set[int] = set()
        normalized = []
        for item in buffers:
            name = str(item["name"])
            size = int(item["bytes"])
            start = int(item["start_phase"])
            end = int(item["end_phase"])
            if size < 0 or start < 0 or end < start:
                raise ValueError(f"invalid buffer lifetime: {name}")
            normalized.append({"name": name, "bytes": size, "start_phase": start,
                               "end_phase": end, "owner": item.get("owner", "unknown")})
            phases.update(range(start, end + 1))

        timeline = []
        fixed_total = sum(fixed.values())
        for phase in sorted(phases or {0}):
            live = [item for item in normalized
                    if item["start_phase"] <= phase <= item["end_phase"]]
            live_bytes = sum(item["bytes"] for item in live)
            timeline.append({"phase": phase, "live_buffer_bytes": live_bytes,
                             "estimated_total_bytes": live_bytes + fixed_total,
                             "live_buffers": [item["name"] for item in live]})
        peak = max(timeline, key=lambda row: row["estimated_total_bytes"])

        map_result = parse_map(args.map_path) if args.map_path else None
        if map_result:
            capacity = map_result["capacity_bytes"]
        estimate_ok = peak["estimated_total_bytes"] <= capacity
        map_ok = map_result is None or map_result["used_bytes"] <= map_result["capacity_bytes"]
        warnings = []
        if map_result and peak["estimated_total_bytes"] < map_result["used_bytes"]:
            warnings.append("lifetime manifest is incomplete: estimate is below linked SRAM used")
        if not map_result:
            warnings.append("no .map supplied; SRAM capacity/use is manifest-only")
        warnings.append("linker stack reservation is not runtime high-water proof")

        report = {
            "status": "PASS" if estimate_ok and map_ok else "FAIL",
            "manifest": str(args.manifest.resolve()),
            "capacity_bytes": capacity,
            "fixed_bytes": fixed,
            "peak_live": peak,
            "timeline": timeline,
            "map_sram": map_result,
            "preferred_existing_build_basis": "map_sram.used_bytes" if map_result else
                                                "manifest estimated_total_bytes",
            "warnings": warnings,
            "board": "NOT_RUN",
        }
        print(json.dumps(report, ensure_ascii=False, indent=2))
        if args.json:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                                 encoding="utf-8")
        return 0 if report["status"] == "PASS" else 1
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())

