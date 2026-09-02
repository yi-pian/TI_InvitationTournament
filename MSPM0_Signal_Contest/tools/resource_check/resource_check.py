#!/usr/bin/env python3
"""Validate simple resource manifests and report conflicting owners."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


RESOURCE_TYPES = {"GPIO", "PIN", "ADC", "DAC", "SPI", "UART", "I2C", "DMA",
                  "TIMER", "EVENT", "IRQ", "OPA", "GPAMP", "COMP"}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", nargs="+", type=Path)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()

    errors: list[str] = []
    warnings: list[str] = []
    claims: dict[tuple[str, str], list[dict]] = {}
    projects: list[str] = []

    for path in args.manifest:
        try:
            data = json.loads(path.read_text(encoding="utf-8-sig"))
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"{path}: {exc}")
            continue
        project = str(data.get("project", path.stem))
        projects.append(project)
        modules = data.get("modules")
        if not isinstance(modules, list):
            errors.append(f"{path}: modules must be a list")
            continue
        for module in modules:
            owner = module.get("name") if isinstance(module, dict) else None
            resources = module.get("resources") if isinstance(module, dict) else None
            if not owner or not isinstance(resources, list):
                errors.append(f"{path}: each module needs name and resources[]")
                continue
            for resource in resources:
                if not isinstance(resource, dict):
                    errors.append(f"{path}: {owner}: resource must be object")
                    continue
                kind = str(resource.get("type", "")).upper()
                identifier = str(resource.get("id", ""))
                if kind not in RESOURCE_TYPES or not identifier:
                    errors.append(f"{path}: {owner}: invalid type/id {kind}/{identifier}")
                    continue
                claim = {
                    "project": project,
                    "owner": owner,
                    "shareable": bool(resource.get("shareable", False)),
                    "config": resource.get("config", {}),
                    "source": str(path),
                }
                claims.setdefault((kind, identifier), []).append(claim)

    conflicts: list[dict] = []
    for (kind, identifier), owners in sorted(claims.items()):
        if len(owners) < 2:
            continue
        if not all(owner["shareable"] for owner in owners):
            conflicts.append({"type": kind, "id": identifier,
                              "reason": "multiple owners and not all claims are shareable",
                              "claims": owners})
            continue
        configs = {json.dumps(owner["config"], sort_keys=True, ensure_ascii=False)
                   for owner in owners}
        if len(configs) != 1:
            conflicts.append({"type": kind, "id": identifier,
                              "reason": "shared resource has incompatible config",
                              "claims": owners})
        else:
            warnings.append(f"shared {kind}/{identifier}: " +
                            ", ".join(owner["owner"] for owner in owners))

    report = {
        "status": "PASS" if not errors and not conflicts else "FAIL",
        "projects": projects,
        "resource_claims": sum(len(value) for value in claims.values()),
        "unique_resources": len(claims),
        "conflicts": conflicts,
        "warnings": warnings,
        "errors": errors,
        "board": "NOT_RUN",
    }
    print(json.dumps(report, ensure_ascii=False, indent=2))
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                             encoding="utf-8")
    return 0 if report["status"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())

