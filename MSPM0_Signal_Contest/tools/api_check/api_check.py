#!/usr/bin/env python3
"""Find Signal* call/reference names that do not exist in selected C/H source roots."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


CALL_RE = re.compile(r"\b(Signal[A-Z][A-Za-z0-9_]+)\s*\(")
SYMBOL_RE = re.compile(r"\b(Signal[A-Z][A-Za-z0-9_]+)\b")
PSEUDOCODE_MARKERS = ("APPLICATION PSEUDOCODE", "ILLUSTRATIVE SNIPPET")


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig", errors="replace")


def source_files(roots: list[Path]):
    for root in roots:
        for path in root.rglob("*"):
            if path.suffix.lower() in {".c", ".h"} and not any(
                    part in {"build", "Debug", "Release", "Objects", "Listings"}
                    for part in path.parts):
                yield path


def reference_files(roots: list[Path]):
    for root in roots:
        for path in root.rglob("*"):
            if path.suffix.lower() in {".c", ".h", ".md"} and not any(
                    part in {"build", "Debug", "Release", "Objects", "Listings"}
                    for part in path.parts):
                yield path


def markdown_refs(text: str) -> set[str]:
    refs: set[str] = set()
    in_fence = False
    pseudocode_fence = False
    for line in text.splitlines():
        if line.lstrip().startswith("```") or line.lstrip().startswith("~~~"):
            if not in_fence:
                pseudocode_fence = any(marker in line for marker in PSEUDOCODE_MARKERS)
            in_fence = not in_fence
            if not in_fence:
                pseudocode_fence = False
            continue
        if pseudocode_fence:
            continue
        refs.update(SYMBOL_RE.findall(line))
    return refs


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", action="append", type=Path, required=True,
                        help="repeatable root containing authoritative .c/.h")
    parser.add_argument("--reference-root", action="append", type=Path, required=True,
                        help="repeatable root containing README/example/recipe/code references")
    parser.add_argument("--allow", action="append", default=[], help="known external symbol")
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()

    source_roots = [p.resolve() for p in args.source_root]
    reference_roots = [p.resolve() for p in args.reference_root]
    source_text = "\n".join(read(path) for path in source_files(source_roots))
    defined = set(SYMBOL_RE.findall(source_text)) | set(args.allow)
    refs: dict[str, set[str]] = {}

    for path in reference_files(reference_roots):
        text = read(path)
        names = markdown_refs(text) if path.suffix.lower() == ".md" else set(CALL_RE.findall(text))
        for name in names:
            refs.setdefault(name, set()).add(str(path))

    missing = {name: sorted(paths) for name, paths in sorted(refs.items()) if name not in defined}
    report = {
        "status": "PASS" if not missing else "FAIL",
        "source_roots": [str(p) for p in source_roots],
        "reference_roots": [str(p) for p in reference_roots],
        "defined_signal_symbols": len(defined),
        "referenced_signal_symbols": len(refs),
        "missing": missing,
    }
    print(json.dumps(report, ensure_ascii=False, indent=2))
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                             encoding="utf-8")
    return 0 if not missing else 1


if __name__ == "__main__":
    sys.exit(main())

