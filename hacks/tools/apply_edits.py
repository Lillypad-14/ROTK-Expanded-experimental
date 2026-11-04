"""Apply exact text replacements listed in a JSON file: safe multi-file edits for agents.

    uv run python hacks/tools/apply_edits.py edits.json [--check]

edits.json: a list of {"file": path, "old": text, "new": text, "count": 1}. "count" is how many times
old must occur (default 1; 0 means every occurrence, at least one). Paths are relative to the
current directory. JSON strings keep "\\n" and quotes exact, unlike shell heredocs.

All edits are checked before any file is written: one miss writes nothing. A miss prints the
closest lines of the file, so the fix is a copy away. --check only reports.
"""

from __future__ import annotations

import argparse
import difflib
import json
from pathlib import Path
import sys


def nearest(text: str, old: str) -> str:
    """The file's lines most like the first line of old, with line numbers."""
    first = next((line for line in old.splitlines() if line.strip()), old)
    lines = text.splitlines()
    scored = sorted(range(len(lines)), key=lambda i: -difflib.SequenceMatcher(None, lines[i], first).ratio())[:3]
    return "\n".join(f"      {i + 1}: {lines[i]}" for i in sorted(scored))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("edits", type=Path)
    parser.add_argument("--check", action="store_true", help="report only, write nothing")
    args = parser.parse_args()
    edits = json.loads(args.edits.read_text(encoding="utf-8"))
    files: dict[Path, str] = {}
    problems = []
    for n, edit in enumerate(edits):
        path = Path(edit["file"])
        if path not in files:
            if not path.exists():
                problems.append(f"#{n} {path}: no such file")
                continue
            with open(path, encoding="utf-8", newline="") as handle:  # keep the file's line endings
                files[path] = handle.read()
        text, old, count = files[path], edit["old"], edit.get("count", 1)
        new = edit["new"]
        if "\r\n" in text and "\r\n" not in old:  # a CRLF file: match and write its line endings
            old, new = old.replace("\n", "\r\n"), new.replace("\n", "\r\n")
        found = text.count(old)
        if found == 0 or (count and found != count):
            problems.append(f"#{n} {path}: old text occurs {found} times, expected {count or 'at least 1'}. "
                            f"Nearest lines:\n{nearest(text, old)}")
            continue
        files[path] = text.replace(old, new) if count == 0 else text.replace(old, new, count)
    if problems:
        print("\n".join(problems))
        sys.exit(f"{len(problems)} of {len(edits)} edits failed, nothing written")
    if not args.check:
        for path, text in files.items():
            with open(path, "w", encoding="utf-8", newline="") as handle:
                handle.write(text)
    print(f"{'Checked' if args.check else 'Applied'} {len(edits)} edits in {len(files)} files")


if __name__ == "__main__":
    main()
