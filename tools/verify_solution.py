#!/usr/bin/env python3
"""Replay the included witness and check its compact public manifest."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
solution = ROOT / "solution"
world = ROOT / "world"
build_dir = Path(os.environ.get("MOTA_BUILD_DIR", ROOT / "build"))
manifest = json.loads((solution / "manifest.json").read_text())

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()

assert sha256(solution / "49409-input.json") == manifest["witness"]["inputSha256"]
assert sha256(solution / "49409.h5route") == manifest["witness"]["replaySha256"]
assert json.loads((world / "manifest.json").read_text())["schema"] == "mota.native-world.v1"

proc = subprocess.run(
    [str(build_dir / "mota-native-replay"), str(world), str(solution / "49409-input.json")],
    cwd=ROOT,
    check=True,
    text=True,
    capture_output=True,
)
rows = [json.loads(line) for line in proc.stdout.splitlines() if line]
assert rows[-1]["kind"] == "complete"
assert rows[-1]["continuous"] is True
assert rows[-1]["initializations"] == 1
assert rows[-1]["appliedActions"] == manifest["witness"]["actions"]
final = next(row["state"] for row in rows if row.get("appliedActions") == manifest["witness"]["actions"])
assert final["state"]["hp"] == manifest["witness"]["hp"]
assert final["state"]["native_win_reason"] == manifest["category"]
assert final["state"]["won50"] is True
assert final["flags"].get("addhp") == 0
print(json.dumps({"passed": True, "hp": final["state"]["hp"], "actions": rows[-1]["appliedActions"], "simulator": rows[-1]["simulator"]}, ensure_ascii=False))
