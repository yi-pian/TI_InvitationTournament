#!/usr/bin/env python3
"""Regression cases for migration/delete gates."""

from __future__ import annotations

import sys
import tempfile
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/canonical_repository"))
from migration_delete_gates import evaluate  # noqa: E402


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def fixture(action: str, replacement: str, tracked: bool) -> tuple[dict, dict, dict]:
    inventory = {"modules": [{
        "module_id": "legacy_contest_demo",
        "source_files": ["MSPM0_Signal_Contest/demo/signal_demo.c"],
        "public_header": "MSPM0_Signal_Contest/demo/signal_demo.h",
        "api_signature_summary": ["SignalDemo_Run"],
        "verification_status": "BUILD_VERIFIED",
    }]}
    migration = {"entries": [{
        "old_module_id": "legacy_contest_demo",
        "action": action,
        "new_canonical_path": replacement,
        "unpreserved_old_api": [],
    }]}
    lost = {"modules": []}
    if tracked:
        lost["modules"].append({
            "old_module_id": "legacy_contest_demo",
            "old_public_api": ["SignalDemo_Run"],
            "historical_verification": "BUILD_VERIFIED",
            "current_verification": "DRAFT",
            "source_available": False,
            "reimplementation_status": "NOT_STARTED",
        })
    return inventory, migration, lost


def main() -> int:
    results = []
    with tempfile.TemporaryDirectory() as tmp:
        workspace = Path(tmp)
        docs = workspace / "MSPM0_Signal_Contest/docs"
        docs.mkdir(parents=True)
        (docs / "recipe.md").write_text("recipe only", encoding="utf-8")

        inv, mig, lost = fixture("DELETE_REPLACED_BY_CANONICAL", "MSPM0_Signal_Contest/docs/recipe.md", False)
        result = evaluate(workspace, inv, mig, lost)
        require(result["checks"]["EXECUTABLE_MODULE_DELETE_CHECK"]["status"] == "FAIL", "doc-only deletion was not blocked")
        require(result["checks"]["EXECUTABLE_MODULE_LOSS_CHECK"]["status"] == "FAIL", "untracked loss was not blocked")
        results.append({"case": "README/Recipe cannot replace executable", "status": "PASS"})

        inv, mig, lost = fixture("SOURCE_LOST_REIMPLEMENTATION_REQUIRED", "", True)
        result = evaluate(workspace, inv, mig, lost)
        require(result["status"] == "PASS", "tracked source loss should pass the safety gate")
        results.append({"case": "tracked SOURCE_LOST is explicit and non-destructive", "status": "PASS"})

        inv, mig, lost = fixture("SOURCE_LOST_REIMPLEMENTATION_REQUIRED", "", True)
        lost["modules"][0]["current_verification"] = "BUILD_VERIFIED"
        result = evaluate(workspace, inv, mig, lost)
        require(result["checks"]["VERIFICATION_REGRESSION_CHECK"]["status"] == "FAIL", "verification inheritance was not blocked")
        results.append({"case": "historical verification cannot be inherited", "status": "PASS"})

        module = workspace / "MSPM0_Signal_Contest/new_demo"
        module.mkdir(parents=True)
        (module / "signal_demo.c").write_text("int demo;\n", encoding="utf-8")
        (module / "signal_demo.h").write_text("int SignalDemo_New(void);\n", encoding="utf-8")
        inv, mig, lost = fixture("SOURCE_LOST_REIMPLEMENTED", "MSPM0_Signal_Contest/new_demo", True)
        lost["modules"][0].update({
            "reimplementation_status": "COMPLETED",
            "current_verification": "PC_VERIFIED",
            "new_path": "MSPM0_Signal_Contest/new_demo",
        })
        result = evaluate(workspace, inv, mig, lost)
        require(result["checks"]["PUBLIC_API_PRESERVATION_CHECK"]["status"] == "FAIL", "unmapped old API was not blocked")
        results.append({"case": "source-lost reimplementation requires explicit old API mapping", "status": "PASS"})

    print(yaml.safe_dump({"status": "PASS", "cases": results}, allow_unicode=True, sort_keys=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
